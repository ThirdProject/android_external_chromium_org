// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/midi/midi_manager_usb.h"

#include <string>

#include "base/strings/stringprintf.h"
#include "media/midi/usb_midi_device.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {

namespace {

template<typename T, size_t N>
std::vector<T> ToVector(const T (&array)[N]) {
  return std::vector<T>(array, array + N);
}

class Logger {
 public:
  Logger() {}
  ~Logger() {}

  void AddLog(const std::string& message) { log_ += message; }
  std::string TakeLog() {
    std::string result;
    result.swap(log_);
    return result;
  }

 private:
  std::string log_;

  DISALLOW_COPY_AND_ASSIGN(Logger);
};

class FakeUsbMidiDevice : public UsbMidiDevice {
 public:
  explicit FakeUsbMidiDevice(Logger* logger) : logger_(logger) {}
  virtual ~FakeUsbMidiDevice() {}

  virtual std::vector<uint8> GetDescriptor() OVERRIDE {
    logger_->AddLog("UsbMidiDevice::GetDescriptor\n");
    return descriptor_;
  }

  virtual void Send(int endpoint_number,
                    const std::vector<uint8>& data) OVERRIDE {
    logger_->AddLog("UsbMidiDevice::Send ");
    logger_->AddLog(base::StringPrintf("endpoint = %d data =",
                                       endpoint_number));
    for (size_t i = 0; i < data.size(); ++i)
      logger_->AddLog(base::StringPrintf(" 0x%02x", data[i]));
    logger_->AddLog("\n");
  }

  void SetDescriptor(const std::vector<uint8> descriptor) {
    descriptor_ = descriptor;
  }

 private:
  std::vector<uint8> descriptor_;
  Logger* logger_;

  DISALLOW_COPY_AND_ASSIGN(FakeUsbMidiDevice);
};

class FakeMidiManagerClient : public MidiManagerClient {
 public:
  explicit FakeMidiManagerClient(Logger* logger) : logger_(logger) {}
  virtual ~FakeMidiManagerClient() {}

  virtual void ReceiveMidiData(uint32 port_index,
                               const uint8* data,
                               size_t size,
                               double timestamp) OVERRIDE {
    logger_->AddLog("MidiManagerClient::ReceiveMidiData ");
    logger_->AddLog(base::StringPrintf("port_index = %d data =", port_index));
    for (size_t i = 0; i < size; ++i)
      logger_->AddLog(base::StringPrintf(" 0x%02x", data[i]));
    logger_->AddLog("\n");
  }

  virtual void AccumulateMidiBytesSent(size_t size) OVERRIDE {
    logger_->AddLog("MidiManagerClient::AccumulateMidiBytesSent ");
    // Windows has no "%zu".
    logger_->AddLog(base::StringPrintf("size = %u\n",
                                       static_cast<unsigned>(size)));
  }

 private:
  Logger* logger_;

  DISALLOW_COPY_AND_ASSIGN(FakeMidiManagerClient);
};

class TestUsbMidiDeviceFactory : public UsbMidiDevice::Factory {
 public:
  TestUsbMidiDeviceFactory() {}
  virtual ~TestUsbMidiDeviceFactory() {}
  virtual void EnumerateDevices(UsbMidiDeviceDelegate* device,
                                Callback callback) OVERRIDE {
    callback_ = callback;
  }

  Callback callback_;

 private:
  DISALLOW_COPY_AND_ASSIGN(TestUsbMidiDeviceFactory);
};

class MidiManagerUsbTest : public ::testing::Test {
 public:
  MidiManagerUsbTest()
      : initialize_callback_run_(false), initialize_result_(false) {
    scoped_ptr<TestUsbMidiDeviceFactory> factory(new TestUsbMidiDeviceFactory);
    factory_ = factory.get();
    manager_.reset(
        new MidiManagerUsb(factory.PassAs<UsbMidiDevice::Factory>()));
  }
  virtual ~MidiManagerUsbTest() {
    std::string leftover_logs = logger_.TakeLog();
    if (!leftover_logs.empty()) {
      ADD_FAILURE() << "Log should be empty: " << leftover_logs;
    }
  }

 protected:
  void Initialize() {
    manager_->Initialize(base::Bind(&MidiManagerUsbTest::OnInitializeDone,
                                    base::Unretained(this)));
  }

  void OnInitializeDone(bool result) {
    initialize_callback_run_ = true;
    initialize_result_ = result;
  }

  bool initialize_callback_run_;
  bool initialize_result_;

  scoped_ptr<MidiManagerUsb> manager_;
  // Owned by manager_.
  TestUsbMidiDeviceFactory* factory_;
  Logger logger_;

 private:
  DISALLOW_COPY_AND_ASSIGN(MidiManagerUsbTest);
};


TEST_F(MidiManagerUsbTest, Initialize) {
  scoped_ptr<FakeUsbMidiDevice> device(new FakeUsbMidiDevice(&logger_));
  uint8 descriptor[] = {
    0x12, 0x01, 0x10, 0x01, 0x00, 0x00, 0x00, 0x08, 0x86, 0x1a,
    0x2d, 0x75, 0x54, 0x02, 0x00, 0x02, 0x00, 0x01, 0x09, 0x02,
    0x75, 0x00, 0x02, 0x01, 0x00, 0x80, 0x30, 0x09, 0x04, 0x00,
    0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x09, 0x24, 0x01, 0x00,
    0x01, 0x09, 0x00, 0x01, 0x01, 0x09, 0x04, 0x01, 0x00, 0x02,
    0x01, 0x03, 0x00, 0x00, 0x07, 0x24, 0x01, 0x00, 0x01, 0x51,
    0x00, 0x06, 0x24, 0x02, 0x01, 0x02, 0x00, 0x06, 0x24, 0x02,
    0x01, 0x03, 0x00, 0x06, 0x24, 0x02, 0x02, 0x06, 0x00, 0x09,
    0x24, 0x03, 0x01, 0x07, 0x01, 0x06, 0x01, 0x00, 0x09, 0x24,
    0x03, 0x02, 0x04, 0x01, 0x02, 0x01, 0x00, 0x09, 0x24, 0x03,
    0x02, 0x05, 0x01, 0x03, 0x01, 0x00, 0x09, 0x05, 0x02, 0x02,
    0x20, 0x00, 0x00, 0x00, 0x00, 0x06, 0x25, 0x01, 0x02, 0x02,
    0x03, 0x09, 0x05, 0x82, 0x02, 0x20, 0x00, 0x00, 0x00, 0x00,
    0x05, 0x25, 0x01, 0x01, 0x07,
  };
  device->SetDescriptor(ToVector(descriptor));

  Initialize();
  ScopedVector<UsbMidiDevice> devices;
  devices.push_back(device.release());
  EXPECT_FALSE(initialize_callback_run_);
  factory_->callback_.Run(true, &devices);
  EXPECT_TRUE(initialize_callback_run_);
  EXPECT_TRUE(initialize_result_);

  ASSERT_EQ(1u, manager_->input_ports().size());
  ASSERT_EQ(2u, manager_->output_ports().size());
  ASSERT_TRUE(manager_->input_stream());
  std::vector<UsbMidiInputStream::JackUniqueKey> keys =
      manager_->input_stream()->RegisteredJackKeysForTesting();
  ASSERT_EQ(2u, manager_->output_streams().size());
  EXPECT_EQ(2u, manager_->output_streams()[0]->jack().jack_id);
  EXPECT_EQ(3u, manager_->output_streams()[1]->jack().jack_id);
  ASSERT_EQ(1u, keys.size());
  EXPECT_EQ(2, keys[0].endpoint_number);

  EXPECT_EQ("UsbMidiDevice::GetDescriptor\n", logger_.TakeLog());
}

TEST_F(MidiManagerUsbTest, InitializeFail) {
  Initialize();

  EXPECT_FALSE(initialize_callback_run_);
  factory_->callback_.Run(false, NULL);
  EXPECT_TRUE(initialize_callback_run_);
  EXPECT_FALSE(initialize_result_);
}

TEST_F(MidiManagerUsbTest, InitializeFailBecauseOfInvalidDescriptor) {
  scoped_ptr<FakeUsbMidiDevice> device(new FakeUsbMidiDevice(&logger_));
  uint8 descriptor[] = {0x04};
  device->SetDescriptor(ToVector(descriptor));

  Initialize();
  ScopedVector<UsbMidiDevice> devices;
  devices.push_back(device.release());
  EXPECT_FALSE(initialize_callback_run_);
  factory_->callback_.Run(true, &devices);
  EXPECT_TRUE(initialize_callback_run_);
  EXPECT_FALSE(initialize_result_);
  EXPECT_EQ("UsbMidiDevice::GetDescriptor\n", logger_.TakeLog());
}

TEST_F(MidiManagerUsbTest, Send) {
  scoped_ptr<FakeUsbMidiDevice> device(new FakeUsbMidiDevice(&logger_));
  FakeMidiManagerClient client(&logger_);
  uint8 descriptor[] = {
    0x12, 0x01, 0x10, 0x01, 0x00, 0x00, 0x00, 0x08, 0x86, 0x1a,
    0x2d, 0x75, 0x54, 0x02, 0x00, 0x02, 0x00, 0x01, 0x09, 0x02,
    0x75, 0x00, 0x02, 0x01, 0x00, 0x80, 0x30, 0x09, 0x04, 0x00,
    0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x09, 0x24, 0x01, 0x00,
    0x01, 0x09, 0x00, 0x01, 0x01, 0x09, 0x04, 0x01, 0x00, 0x02,
    0x01, 0x03, 0x00, 0x00, 0x07, 0x24, 0x01, 0x00, 0x01, 0x51,
    0x00, 0x06, 0x24, 0x02, 0x01, 0x02, 0x00, 0x06, 0x24, 0x02,
    0x01, 0x03, 0x00, 0x06, 0x24, 0x02, 0x02, 0x06, 0x00, 0x09,
    0x24, 0x03, 0x01, 0x07, 0x01, 0x06, 0x01, 0x00, 0x09, 0x24,
    0x03, 0x02, 0x04, 0x01, 0x02, 0x01, 0x00, 0x09, 0x24, 0x03,
    0x02, 0x05, 0x01, 0x03, 0x01, 0x00, 0x09, 0x05, 0x02, 0x02,
    0x20, 0x00, 0x00, 0x00, 0x00, 0x06, 0x25, 0x01, 0x02, 0x02,
    0x03, 0x09, 0x05, 0x82, 0x02, 0x20, 0x00, 0x00, 0x00, 0x00,
    0x05, 0x25, 0x01, 0x01, 0x07,
  };

  device->SetDescriptor(ToVector(descriptor));
  uint8 data[] = {
    0x90, 0x45, 0x7f,
    0xf0, 0x00, 0x01, 0xf7,
  };

  Initialize();
  ScopedVector<UsbMidiDevice> devices;
  devices.push_back(device.release());
  EXPECT_FALSE(initialize_callback_run_);
  factory_->callback_.Run(true, &devices);
  ASSERT_TRUE(initialize_callback_run_);
  ASSERT_TRUE(initialize_result_);
  ASSERT_EQ(2u, manager_->output_streams().size());

  manager_->DispatchSendMidiData(&client, 1, ToVector(data), 0);
  EXPECT_EQ("UsbMidiDevice::GetDescriptor\n"
            "UsbMidiDevice::Send endpoint = 2 data = "
            "0x19 0x90 0x45 0x7f "
            "0x14 0xf0 0x00 0x01 "
            "0x15 0xf7 0x00 0x00\n"
            "MidiManagerClient::AccumulateMidiBytesSent size = 7\n",
            logger_.TakeLog());
}

TEST_F(MidiManagerUsbTest, Receive) {
  scoped_ptr<FakeUsbMidiDevice> device(new FakeUsbMidiDevice(&logger_));
  FakeMidiManagerClient client(&logger_);
  uint8 descriptor[] = {
    0x12, 0x01, 0x10, 0x01, 0x00, 0x00, 0x00, 0x08, 0x86, 0x1a,
    0x2d, 0x75, 0x54, 0x02, 0x00, 0x02, 0x00, 0x01, 0x09, 0x02,
    0x75, 0x00, 0x02, 0x01, 0x00, 0x80, 0x30, 0x09, 0x04, 0x00,
    0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x09, 0x24, 0x01, 0x00,
    0x01, 0x09, 0x00, 0x01, 0x01, 0x09, 0x04, 0x01, 0x00, 0x02,
    0x01, 0x03, 0x00, 0x00, 0x07, 0x24, 0x01, 0x00, 0x01, 0x51,
    0x00, 0x06, 0x24, 0x02, 0x01, 0x02, 0x00, 0x06, 0x24, 0x02,
    0x01, 0x03, 0x00, 0x06, 0x24, 0x02, 0x02, 0x06, 0x00, 0x09,
    0x24, 0x03, 0x01, 0x07, 0x01, 0x06, 0x01, 0x00, 0x09, 0x24,
    0x03, 0x02, 0x04, 0x01, 0x02, 0x01, 0x00, 0x09, 0x24, 0x03,
    0x02, 0x05, 0x01, 0x03, 0x01, 0x00, 0x09, 0x05, 0x02, 0x02,
    0x20, 0x00, 0x00, 0x00, 0x00, 0x06, 0x25, 0x01, 0x02, 0x02,
    0x03, 0x09, 0x05, 0x82, 0x02, 0x20, 0x00, 0x00, 0x00, 0x00,
    0x05, 0x25, 0x01, 0x01, 0x07,
  };

  device->SetDescriptor(ToVector(descriptor));
  uint8 data[] = {
    0x09, 0x90, 0x45, 0x7f,
    0x04, 0xf0, 0x00, 0x01,
    0x49, 0x90, 0x88, 0x99,  // This data should be ignored (CN = 4).
    0x05, 0xf7, 0x00, 0x00,
  };

  Initialize();
  ScopedVector<UsbMidiDevice> devices;
  UsbMidiDevice* device_raw = device.get();
  devices.push_back(device.release());
  EXPECT_FALSE(initialize_callback_run_);
  factory_->callback_.Run(true, &devices);
  ASSERT_TRUE(initialize_callback_run_);
  ASSERT_TRUE(initialize_result_);

  manager_->StartSession(&client);
  manager_->ReceiveUsbMidiData(device_raw, 2, data, arraysize(data), 0);
  manager_->EndSession(&client);

  EXPECT_EQ("UsbMidiDevice::GetDescriptor\n"
            "MidiManagerClient::ReceiveMidiData port_index = 0 "
            "data = 0x90 0x45 0x7f\n"
            "MidiManagerClient::ReceiveMidiData port_index = 0 "
            "data = 0xf0 0x00 0x01\n"
            "MidiManagerClient::ReceiveMidiData port_index = 0 data = 0xf7\n",
            logger_.TakeLog());
}

}  // namespace

}  // namespace media
