// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/network/managed_state.h"

#include "base/logging.h"
#include "base/values.h"
#include "chromeos/network/device_state.h"
#include "chromeos/network/favorite_state.h"
#include "chromeos/network/network_event_log.h"
#include "chromeos/network/network_state.h"
#include "chromeos/network/shill_property_util.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace chromeos {

bool ManagedState::Matches(const NetworkTypePattern& pattern) const {
  return pattern.MatchesType(type());
}

ManagedState::ManagedState(ManagedType type, const std::string& path)
    : managed_type_(type),
      path_(path),
      update_received_(false),
      update_requested_(false) {
}

ManagedState::~ManagedState() {
}

ManagedState* ManagedState::Create(ManagedType type, const std::string& path) {
  switch (type) {
    case MANAGED_TYPE_NETWORK:
      return new NetworkState(path);
    case MANAGED_TYPE_FAVORITE:
      return new FavoriteState(path);
    case MANAGED_TYPE_DEVICE:
      return new DeviceState(path);
  }
  return NULL;
}

NetworkState* ManagedState::AsNetworkState() {
  if (managed_type() == MANAGED_TYPE_NETWORK)
    return static_cast<NetworkState*>(this);
  return NULL;
}

DeviceState* ManagedState::AsDeviceState() {
  if (managed_type() == MANAGED_TYPE_DEVICE)
    return static_cast<DeviceState*>(this);
  return NULL;
}

FavoriteState* ManagedState::AsFavoriteState() {
  if (managed_type() == MANAGED_TYPE_FAVORITE)
    return static_cast<FavoriteState*>(this);
  return NULL;
}

bool ManagedState::InitialPropertiesReceived(
    const base::DictionaryValue& properties) {
  return false;
}

bool ManagedState::ManagedStatePropertyChanged(const std::string& key,
                                               const base::Value& value) {
  if (key == flimflam::kNameProperty) {
    return GetStringValue(key, value, &name_);
  } else if (key == flimflam::kTypeProperty) {
    return GetStringValue(key, value, &type_);
  }
  return false;
}

bool ManagedState::GetBooleanValue(const std::string& key,
                                   const base::Value& value,
                                   bool* out_value) {
  bool new_value;
  if (!value.GetAsBoolean(&new_value)) {
    NET_LOG_ERROR("Error parsing state value", path() + "." + key);
    return false;
  }
  if (*out_value == new_value)
    return false;
  *out_value = new_value;
  return true;
}

bool ManagedState::GetIntegerValue(const std::string& key,
                                   const base::Value& value,
                                   int* out_value) {
  int new_value;
  if (!value.GetAsInteger(&new_value)) {
    NET_LOG_ERROR("Error parsing state value", path() + "." + key);
    return false;
  }
  if (*out_value == new_value)
    return false;
  *out_value = new_value;
  return true;
}

bool ManagedState::GetStringValue(const std::string& key,
                                  const base::Value& value,
                                  std::string* out_value) {
  std::string new_value;
  if (!value.GetAsString(&new_value)) {
    NET_LOG_ERROR("Error parsing state value", path() + "." + key);
    return false;
  }
  if (*out_value == new_value)
    return false;
  *out_value = new_value;
  return true;
}

bool ManagedState::GetUInt32Value(const std::string& key,
                                  const base::Value& value,
                                  uint32* out_value) {
  // base::Value restricts the number types to BOOL, INTEGER, and DOUBLE only.
  // uint32 will automatically get converted to a double, which is why we try
  // to obtain the value as a double (see dbus/values_util.h).
  uint32 new_value;
  double double_value;
  if (!value.GetAsDouble(&double_value) || double_value < 0) {
    NET_LOG_ERROR("Error parsing state value", path() + "." + key);
    return false;
  }
  new_value = static_cast<uint32>(double_value);
  if (*out_value == new_value)
    return false;
  *out_value = new_value;
  return true;
}

}  // namespace chromeos
