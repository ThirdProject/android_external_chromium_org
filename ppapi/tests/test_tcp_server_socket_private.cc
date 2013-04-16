// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/tests/test_tcp_server_socket_private.h"

#include <vector>

#include "ppapi/cpp/pass_ref.h"
#include "ppapi/cpp/private/net_address_private.h"
#include "ppapi/cpp/private/tcp_server_socket_private.h"
#include "ppapi/cpp/private/tcp_socket_private.h"
#include "ppapi/tests/test_utils.h"
#include "ppapi/tests/testing_instance.h"

using pp::NetAddressPrivate;
using pp::TCPServerSocketPrivate;
using pp::TCPSocketPrivate;

namespace {

const uint16_t kPortScanFrom = 1024;
const uint16_t kPortScanTo = 4096;

}  // namespace

REGISTER_TEST_CASE(TCPServerSocketPrivate);

TestTCPServerSocketPrivate::TestTCPServerSocketPrivate(
    TestingInstance* instance) : TestCase(instance) {
}

bool TestTCPServerSocketPrivate::Init() {
  bool tcp_server_socket_private_is_available =
      TCPServerSocketPrivate::IsAvailable();
  if (!tcp_server_socket_private_is_available) {
    instance_->AppendError(
        "PPB_TCPServerSocket_Private interface not available");
  }

  bool tcp_socket_private_is_available = TCPSocketPrivate::IsAvailable();
  if (!tcp_socket_private_is_available)
    instance_->AppendError("PPB_TCPSocket_Private interface not available");

  bool net_address_private_is_available = NetAddressPrivate::IsAvailable();
  if (!net_address_private_is_available)
    instance_->AppendError("PPB_NetAddress_Private interface not available");

  bool init_host_port = GetLocalHostPort(instance_->pp_instance(),
                                         &host_, &port_);
  if (!init_host_port)
    instance_->AppendError("Can't init host and port");

  return tcp_server_socket_private_is_available &&
      tcp_socket_private_is_available &&
      net_address_private_is_available &&
      init_host_port &&
      CheckTestingInterface() &&
      EnsureRunningOverHTTP();
}

void TestTCPServerSocketPrivate::RunTests(const std::string& filter) {
  RUN_CALLBACK_TEST(TestTCPServerSocketPrivate, Listen, filter);
  RUN_CALLBACK_TEST(TestTCPServerSocketPrivate, Backlog, filter);
}

std::string TestTCPServerSocketPrivate::GetLocalAddress(
    PP_NetAddress_Private* address) {
  TCPSocketPrivate socket(instance_);
  TestCompletionCallback callback(instance_->pp_instance(), callback_type());
  callback.WaitForResult(
      socket.Connect(host_.c_str(), port_, callback.GetCallback()));
  CHECK_CALLBACK_BEHAVIOR(callback);
  ASSERT_EQ(PP_OK, callback.result());
  ASSERT_TRUE(socket.GetLocalAddress(address));
  socket.Disconnect();
  PASS();
}

std::string TestTCPServerSocketPrivate::SyncRead(TCPSocketPrivate* socket,
                                                 char* buffer,
                                                 size_t num_bytes) {
  while (num_bytes > 0) {
    TestCompletionCallback callback(instance_->pp_instance(), callback_type());
    callback.WaitForResult(
        socket->Read(buffer, num_bytes, callback.GetCallback()));
    CHECK_CALLBACK_BEHAVIOR(callback);
    ASSERT_TRUE(callback.result() >= 0);
    buffer += callback.result();
    num_bytes -= callback.result();
  }
  PASS();
}

std::string TestTCPServerSocketPrivate::SyncWrite(TCPSocketPrivate* socket,
                                                  const char* buffer,
                                                  size_t num_bytes) {
  while (num_bytes > 0) {
    TestCompletionCallback callback(instance_->pp_instance(), callback_type());
    callback.WaitForResult(
        socket->Write(buffer, num_bytes, callback.GetCallback()));
    CHECK_CALLBACK_BEHAVIOR(callback);
    ASSERT_TRUE(callback.result() >= 0);
    buffer += callback.result();
    num_bytes -= callback.result();
  }
  PASS();
}

std::string TestTCPServerSocketPrivate::SyncConnect(
    TCPSocketPrivate* socket,
    PP_NetAddress_Private* address) {
  TestCompletionCallback callback(instance_->pp_instance(), callback_type());
  callback.WaitForResult(
      socket->ConnectWithNetAddress(address, callback.GetCallback()));
  CHECK_CALLBACK_BEHAVIOR(callback);
  ASSERT_EQ(PP_OK, callback.result());
  PASS();
}

void TestTCPServerSocketPrivate::ForceConnect(TCPSocketPrivate* socket,
                                              PP_NetAddress_Private* address) {
  std::string error_message;
  do {
    error_message = SyncConnect(socket, address);
  } while (!error_message.empty());
}

std::string TestTCPServerSocketPrivate::SyncListen(
    TCPServerSocketPrivate* socket,
    PP_NetAddress_Private* address,
    int32_t backlog) {
  PP_NetAddress_Private base_address;
  ASSERT_SUBTEST_SUCCESS(GetLocalAddress(&base_address));

  // TODO (ygorshenin): find more efficient way to select available
  // ports.
  bool is_free_port_found = false;
  for (uint16_t port = kPortScanFrom; port < kPortScanTo; ++port) {
    if (!NetAddressPrivate::ReplacePort(base_address, port, address))
      return ReportError("PPB_NetAddress_Private::ReplacePort", 0);

    TestCompletionCallback callback(instance_->pp_instance(), callback_type());
    callback.WaitForResult(
        socket->Listen(address, backlog, callback.GetCallback()));
    CHECK_CALLBACK_BEHAVIOR(callback);
    if (callback.result() == PP_OK) {
      is_free_port_found = true;
      break;
    }
  }

  ASSERT_TRUE(is_free_port_found);
  PASS();
}

std::string TestTCPServerSocketPrivate::TestListen() {
  static const int kBacklog = 2;

  TCPServerSocketPrivate server_socket(instance_);

  PP_NetAddress_Private address;
  ASSERT_SUBTEST_SUCCESS(SyncListen(&server_socket, &address, kBacklog));

  // We can't use a blocking callback for Accept, because it will wait forever
  // for the client to connect, since the client connects after.
  TestCompletionCallback accept_callback(instance_->pp_instance(),
                                         PP_REQUIRED);
  // We need to make sure there's a message loop to run accept_callback on.
  pp::MessageLoop current_thread_loop(pp::MessageLoop::GetCurrent());
  if (current_thread_loop.is_null() && testing_interface_->IsOutOfProcess()) {
    current_thread_loop = pp::MessageLoop(instance_);
    current_thread_loop.AttachToCurrentThread();
  }

  PP_Resource resource;
  int32_t accept_rv = server_socket.Accept(&resource,
                                           accept_callback.GetCallback());

  TCPSocketPrivate client_socket(instance_);
  ForceConnect(&client_socket, &address);

  accept_callback.WaitForResult(accept_rv);
  CHECK_CALLBACK_BEHAVIOR(accept_callback);
  ASSERT_EQ(PP_OK, accept_callback.result());

  ASSERT_TRUE(resource != 0);
  TCPSocketPrivate accepted_socket(pp::PassRef(), resource);

  const char kSentByte = 'a';
  ASSERT_SUBTEST_SUCCESS(SyncWrite(&client_socket,
                                   &kSentByte,
                                   sizeof(kSentByte)));

  char received_byte;
  ASSERT_SUBTEST_SUCCESS(SyncRead(&accepted_socket,
                                  &received_byte,
                                  sizeof(received_byte)));
  ASSERT_EQ(kSentByte, received_byte);

  accepted_socket.Disconnect();
  client_socket.Disconnect();
  server_socket.StopListening();

  PASS();
}

std::string TestTCPServerSocketPrivate::TestBacklog() {
  static const size_t kBacklog = 5;

  TCPServerSocketPrivate server_socket(instance_);

  PP_NetAddress_Private address;
  ASSERT_SUBTEST_SUCCESS(SyncListen(&server_socket, &address, 2 * kBacklog));

  std::vector<TCPSocketPrivate*> client_sockets(kBacklog);
  std::vector<TestCompletionCallback*> connect_callbacks(kBacklog);
  std::vector<int32_t> connect_rv(kBacklog);
  for (size_t i = 0; i < kBacklog; ++i) {
    client_sockets[i] = new TCPSocketPrivate(instance_);
    connect_callbacks[i] = new TestCompletionCallback(instance_->pp_instance(),
                                                      callback_type());
    connect_rv[i] = client_sockets[i]->ConnectWithNetAddress(
        &address,
        connect_callbacks[i]->GetCallback());
  }

  std::vector<PP_Resource> resources(kBacklog);
  std::vector<TCPSocketPrivate*> accepted_sockets(kBacklog);
  for (size_t i = 0; i < kBacklog; ++i) {
    TestCompletionCallback callback(instance_->pp_instance(), callback_type());
    callback.WaitForResult(
        server_socket.Accept(&resources[i], callback.GetCallback()));
    CHECK_CALLBACK_BEHAVIOR(callback);
    ASSERT_EQ(PP_OK, callback.result());

    ASSERT_TRUE(resources[i] != 0);
    accepted_sockets[i] = new TCPSocketPrivate(pp::PassRef(), resources[i]);
  }

  for (size_t i = 0; i < kBacklog; ++i) {
    connect_callbacks[i]->WaitForResult(connect_rv[i]);
    CHECK_CALLBACK_BEHAVIOR(*connect_callbacks[i]);
    ASSERT_EQ(PP_OK, connect_callbacks[i]->result());
  }

  for (size_t i = 0; i < kBacklog; ++i) {
    const char byte = 'a' + i;
    ASSERT_SUBTEST_SUCCESS(SyncWrite(client_sockets[i], &byte, sizeof(byte)));
  }

  bool byte_received[kBacklog] = {};
  for (size_t i = 0; i < kBacklog; ++i) {
    char byte;
    ASSERT_SUBTEST_SUCCESS(SyncRead(accepted_sockets[i], &byte, sizeof(byte)));
    const size_t index = byte - 'a';
    ASSERT_FALSE(byte_received[index]);
    byte_received[index] = true;
  }

  for (size_t i = 0; i < kBacklog; ++i) {
    client_sockets[i]->Disconnect();
    delete client_sockets[i];
    delete connect_callbacks[i];
    accepted_sockets[i]->Disconnect();
    delete accepted_sockets[i];
  }

  server_socket.StopListening();
  PASS();
}
