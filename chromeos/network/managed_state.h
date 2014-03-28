// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_NETWORK_MANAGED_STATE_H_
#define CHROMEOS_NETWORK_MANAGED_STATE_H_

#include <string>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "chromeos/chromeos_export.h"

namespace base {
class Value;
class DictionaryValue;
}

namespace chromeos {

class DeviceState;
class FavoriteState;
class NetworkState;
class NetworkTypePattern;

// Base class for states managed by NetworkStateManger which are associated
// with a Shill path (e.g. service path or device path).
class CHROMEOS_EXPORT ManagedState {
 public:
  enum ManagedType {
    MANAGED_TYPE_NETWORK,
    MANAGED_TYPE_FAVORITE,
    MANAGED_TYPE_DEVICE
  };

  virtual ~ManagedState();

  // This will construct and return a new instance of the appropriate class
  // based on |type|.
  static ManagedState* Create(ManagedType type, const std::string& path);

  // Returns the specific class pointer if this is the correct type, or
  // NULL if it is not.
  NetworkState* AsNetworkState();
  DeviceState* AsDeviceState();
  FavoriteState* AsFavoriteState();

  // Called by NetworkStateHandler when a property was received. The return
  // value indicates if the state changed and is used to reduce the number of
  // notifications. The only guarantee however is: If the return value is false
  // then the state wasn't modified. This might happen because of
  // * |key| was not recognized.
  // * |value| was not parsed successfully.
  // * |value| is equal to the cached property value.
  // If the return value is true, the state might or might not be modified.
  virtual bool PropertyChanged(const std::string& key,
                               const base::Value& value) = 0;

  // Called by NetworkStateHandler after all calls to PropertyChanged for the
  // initial set of properties. Used to update state requiring multiple
  // properties, e.g. name from hex_ssid in NetworkState.
  // |properties| contains the complete set of initial properties.
  // Returns true if any additional properties are updated.
  virtual bool InitialPropertiesReceived(
      const base::DictionaryValue& properties);

  const ManagedType managed_type() const { return managed_type_; }
  const std::string& path() const { return path_; }
  const std::string& name() const { return name_; }
  const std::string& type() const { return type_; }
  bool update_received() const { return update_received_; }
  void set_update_received() { update_received_ = true; }
  bool update_requested() const { return update_requested_; }
  void set_update_requested(bool update_requested) {
    update_requested_ = update_requested;
  }

  bool Matches(const NetworkTypePattern& pattern) const;

  static std::string TypeToString(ManagedType type);

 protected:
  ManagedState(ManagedType type, const std::string& path);

  // Parses common property keys (name, type).
  bool ManagedStatePropertyChanged(const std::string& key,
                                   const base::Value& value);

  // Helper methods that log warnings and return true if parsing succeeded and
  // the new value does not match the existing output value.
  bool GetBooleanValue(const std::string& key,
                       const base::Value& value,
                       bool* out_value);
  bool GetIntegerValue(const std::string& key,
                       const base::Value& value,
                       int* out_value);
  bool GetStringValue(const std::string& key,
                      const base::Value& value,
                      std::string* out_value);
  bool GetUInt32Value(const std::string& key,
                      const base::Value& value,
                      uint32* out_value);

  void set_name(const std::string& name) { name_ = name; }

 private:
  friend class NetworkChangeNotifierChromeosUpdateTest;

  ManagedType managed_type_;

  // The path (e.g. service path or device path) of the managed state object.
  std::string path_;

  // Common properties shared by all managed state objects.
  std::string name_;  // shill::kNameProperty
  std::string type_;  // shill::kTypeProperty

  // Set to true when the an update has been received.
  bool update_received_;

  // Tracks when an update has been requested.
  bool update_requested_;

  DISALLOW_COPY_AND_ASSIGN(ManagedState);
};

}  // namespace chromeos

#endif  // CHROMEOS_NETWORK_MANAGED_STATE_H_
