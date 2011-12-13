// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/protocol/connection_tester.h"

#include "base/bind.h"
#include "base/message_loop.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/socket/stream_socket.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace remoting {
namespace protocol {

StreamConnectionTester::StreamConnectionTester(net::StreamSocket* client_socket,
                                               net::StreamSocket* host_socket,
                                               int message_size,
                                               int message_count)
    : message_loop_(MessageLoop::current()),
      host_socket_(host_socket),
      client_socket_(client_socket),
      message_size_(message_size),
      message_count_(message_count),
      test_data_size_(message_size * message_count),
      done_(false),
      write_errors_(0),
      read_errors_(0) {
}

StreamConnectionTester::~StreamConnectionTester() {
}

void StreamConnectionTester::Start() {
  InitBuffers();
  DoRead();
  DoWrite();
}

void StreamConnectionTester::CheckResults() {
  EXPECT_EQ(0, write_errors_);
  EXPECT_EQ(0, read_errors_);

  ASSERT_EQ(test_data_size_, input_buffer_->offset());

  output_buffer_->SetOffset(0);
  ASSERT_EQ(test_data_size_, output_buffer_->size());

  EXPECT_EQ(0, memcmp(output_buffer_->data(),
                      input_buffer_->StartOfBuffer(), test_data_size_));
}

void StreamConnectionTester::Done() {
  done_ = true;
  message_loop_->PostTask(FROM_HERE, MessageLoop::QuitClosure());
}

void StreamConnectionTester::InitBuffers() {
  output_buffer_ = new net::DrainableIOBuffer(
      new net::IOBuffer(test_data_size_), test_data_size_);
  for (int i = 0; i < test_data_size_; ++i) {
    output_buffer_->data()[i] = static_cast<char>(i);
  }

  input_buffer_ = new net::GrowableIOBuffer();
}

void StreamConnectionTester::DoWrite() {
  int result = 1;
  while (result > 0) {
    if (output_buffer_->BytesRemaining() == 0)
      break;

    int bytes_to_write = std::min(output_buffer_->BytesRemaining(),
                                  message_size_);
    result = client_socket_->Write(
        output_buffer_, bytes_to_write,
        base::Bind(&StreamConnectionTester::OnWritten, base::Unretained(this)));
    HandleWriteResult(result);
  }
}

void StreamConnectionTester::OnWritten(int result) {
  HandleWriteResult(result);
  DoWrite();
}

void StreamConnectionTester::HandleWriteResult(int result) {
  if (result <= 0 && result != net::ERR_IO_PENDING) {
    LOG(ERROR) << "Received error " << result << " when trying to write";
    write_errors_++;
    Done();
  } else if (result > 0) {
    output_buffer_->DidConsume(result);
  }
}

void StreamConnectionTester::DoRead() {
  int result = 1;
  while (result > 0) {
    input_buffer_->SetCapacity(input_buffer_->offset() + message_size_);
    result = host_socket_->Read(
        input_buffer_, message_size_,
        base::Bind(&StreamConnectionTester::OnRead, base::Unretained(this)));
    HandleReadResult(result);
  };
}

void StreamConnectionTester::OnRead(int result) {
  HandleReadResult(result);
  if (!done_)
    DoRead();  // Don't try to read again when we are done reading.
}

void StreamConnectionTester::HandleReadResult(int result) {
  if (result <= 0 && result != net::ERR_IO_PENDING) {
    LOG(ERROR) << "Received error " << result << " when trying to read";
    read_errors_++;
    Done();
  } else if (result > 0) {
    // Allocate memory for the next read.
    input_buffer_->set_offset(input_buffer_->offset() + result);
    if (input_buffer_->offset() == test_data_size_)
      Done();
  }
}

DatagramConnectionTester::DatagramConnectionTester(net::Socket* client_socket,
                                                   net::Socket* host_socket,
                                                   int message_size,
                                                   int message_count,
                                                   int delay_ms)
    : message_loop_(MessageLoop::current()),
      host_socket_(host_socket),
      client_socket_(client_socket),
      message_size_(message_size),
      message_count_(message_count),
      delay_ms_(delay_ms),
      done_(false),
      write_errors_(0),
      read_errors_(0),
      packets_sent_(0),
      packets_received_(0),
      bad_packets_received_(0) {
  sent_packets_.resize(message_count_);
}

DatagramConnectionTester::~DatagramConnectionTester() {
}

void DatagramConnectionTester::Start() {
  DoRead();
  DoWrite();
}

void DatagramConnectionTester::CheckResults() {
  EXPECT_EQ(0, write_errors_);
  EXPECT_EQ(0, read_errors_);

  EXPECT_EQ(0, bad_packets_received_);

  // Verify that we've received at least one packet.
  EXPECT_GT(packets_received_, 0);
  LOG(INFO) << "Received " << packets_received_ << " packets out of "
            << message_count_;
}

void DatagramConnectionTester::Done() {
  done_ = true;
  message_loop_->PostTask(FROM_HERE, MessageLoop::QuitClosure());
}

void DatagramConnectionTester::DoWrite() {
  if (packets_sent_ >= message_count_) {
    Done();
    return;
  }

  scoped_refptr<net::IOBuffer> packet(new net::IOBuffer(message_size_));
  for (int i = 0; i < message_size_; ++i) {
    packet->data()[i] = static_cast<char>(i);
  }
  sent_packets_[packets_sent_] = packet;
  // Put index of this packet in the beginning of the packet body.
  memcpy(packet->data(), &packets_sent_, sizeof(packets_sent_));

  int result = client_socket_->Write(
      packet, message_size_,
      base::Bind(&DatagramConnectionTester::OnWritten, base::Unretained(this)));
  HandleWriteResult(result);
}

void DatagramConnectionTester::OnWritten(int result) {
  HandleWriteResult(result);
}

void DatagramConnectionTester::HandleWriteResult(int result) {
  if (result <= 0 && result != net::ERR_IO_PENDING) {
    LOG(ERROR) << "Received error " << result << " when trying to write";
    write_errors_++;
    Done();
  } else if (result > 0) {
    EXPECT_EQ(message_size_, result);
    packets_sent_++;
    message_loop_->PostDelayedTask(FROM_HERE, base::Bind(
        &DatagramConnectionTester::DoWrite, base::Unretained(this)), delay_ms_);
  }
}

void DatagramConnectionTester::DoRead() {
  int result = 1;
  while (result > 0) {
    int kReadSize = message_size_ * 2;
    read_buffer_ = new net::IOBuffer(kReadSize);

    result = host_socket_->Read(
        read_buffer_, kReadSize,
        base::Bind(&DatagramConnectionTester::OnRead, base::Unretained(this)));
    HandleReadResult(result);
  };
}

void DatagramConnectionTester::OnRead(int result) {
  HandleReadResult(result);
  DoRead();
}

void DatagramConnectionTester::HandleReadResult(int result) {
  if (result <= 0 && result != net::ERR_IO_PENDING) {
    // Error will be received after the socket is closed.
    LOG(ERROR) << "Received error " << result << " when trying to read";
    read_errors_++;
    Done();
  } else if (result > 0) {
    packets_received_++;
    if (message_size_ != result) {
      // Invalid packet size;
      bad_packets_received_++;
    } else {
      // Validate packet body.
      int packet_id;
      memcpy(&packet_id, read_buffer_->data(), sizeof(packet_id));
      if (packet_id < 0 || packet_id >= message_count_) {
        bad_packets_received_++;
      } else {
        if (memcmp(read_buffer_->data(), sent_packets_[packet_id]->data(),
                   message_size_) != 0)
          bad_packets_received_++;
      }
    }
  }
}

}  // namespace protocol
}  // namespace remoting
