// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/serial/serial_device_enumerator_linux.h"

#include "base/logging.h"
#include "base/memory/linked_ptr.h"
#include "base/strings/string_number_conversions.h"

namespace device {

namespace {

const char kSerialSubsystem[] = "tty";

const char kHostPathKey[] = "DEVNAME";
const char kHostBusKey[] = "ID_BUS";
const char kVendorIDKey[] = "ID_VENDOR_ID";
const char kProductIDKey[] = "ID_MODEL_ID";
const char kProductNameKey[] = "ID_MODEL";

struct UdevEnumerateDeleter {
  void operator()(udev_enumerate* enumerate) {
    udev_enumerate_unref(enumerate);
  }
};

struct UdevDeviceDeleter {
  void operator()(udev_device* device) { udev_device_unref(device); }
};

typedef scoped_ptr<udev_enumerate, UdevEnumerateDeleter> ScopedUdevEnumeratePtr;
typedef scoped_ptr<udev_device, UdevDeviceDeleter> ScopedUdevDevicePtr;
}

// static
scoped_ptr<SerialDeviceEnumerator> SerialDeviceEnumerator::Create() {
  return scoped_ptr<SerialDeviceEnumerator>(new SerialDeviceEnumeratorLinux());
}

SerialDeviceEnumeratorLinux::SerialDeviceEnumeratorLinux() {
  udev_.reset(udev_new());
}

SerialDeviceEnumeratorLinux::~SerialDeviceEnumeratorLinux() {}

void SerialDeviceEnumeratorLinux::GetDevices(SerialDeviceInfoList* devices) {
  devices->clear();

  ScopedUdevEnumeratePtr enumerate(udev_enumerate_new(udev_.get()));
  if (!enumerate) {
    LOG(ERROR) << "Serial device enumeration failed.";
    return;
  }
  if (udev_enumerate_add_match_subsystem(enumerate.get(), kSerialSubsystem)) {
    LOG(ERROR) << "Serial device enumeration failed.";
    return;
  }
  if (udev_enumerate_scan_devices(enumerate.get())) {
    LOG(ERROR) << "Serial device enumeration failed.";
    return;
  }

  udev_list_entry* entry = udev_enumerate_get_list_entry(enumerate.get());
  for (; entry != NULL; entry = udev_list_entry_get_next(entry)) {
    ScopedUdevDevicePtr device(udev_device_new_from_syspath(
        udev_.get(), udev_list_entry_get_name(entry)));
    // TODO(rockot): There may be a better way to filter serial devices here,
    // but it's not clear what that would be. Udev will list lots of virtual
    // devices with no real endpoint to back them anywhere. The presence of
    // a bus identifier (e.g., "pci" or "usb") seems to be a good heuristic
    // for detecting actual devices.
    const char* path =
        udev_device_get_property_value(device.get(), kHostPathKey);
    const char* bus = udev_device_get_property_value(device.get(), kHostBusKey);
    if (path != NULL && bus != NULL) {
      linked_ptr<SerialDeviceInfo> info(new SerialDeviceInfo());
      info->path = std::string(path);

      const char* vendor_id =
          udev_device_get_property_value(device.get(), kVendorIDKey);
      const char* product_id =
          udev_device_get_property_value(device.get(), kProductIDKey);
      const char* product_name =
          udev_device_get_property_value(device.get(), kProductNameKey);

      uint32 int_value;
      if (vendor_id && base::HexStringToUInt(vendor_id, &int_value))
        info->vendor_id.reset(new uint16(int_value));
      if (product_id && base::HexStringToUInt(product_id, &int_value))
        info->product_id.reset(new uint16(int_value));
      if (product_name)
        info->display_name.reset(new std::string(product_name));

      devices->push_back(info);
    }
  }
}

void SerialDeviceEnumeratorLinux::UdevDeleter::operator()(udev* handle) {
  udev_unref(handle);
}

}  // namespace device
