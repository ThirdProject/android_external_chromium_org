// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_HOST_PAIRING_SCREEN_ACTOR_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_HOST_PAIRING_SCREEN_ACTOR_H_

#include "base/macros.h"

namespace base {
class DictionaryValue;
}

namespace chromeos {

namespace host_pairing {

// Keep these constants synced with corresponding constants defined in
// oobe_screen_host_pairing.js.
// Conxtext keys.
extern const char kContextKeyPage[];
extern const char kContextKeyDeviceName[];
extern const char kContextKeyConfirmationCode[];
extern const char kContextKeyEnrollmentDomain[];
extern const char kContextKeyUpdateProgress[];

// Pages names.
extern const char kPageWelcome[];
extern const char kPageCodeConfirmation[];
extern const char kPageUpdate[];
extern const char kPageEnrollmentIntroduction[];
extern const char kPageEnrollment[];
extern const char kPageEnrollmentError[];
extern const char kPagePairingDone[];

}  // namespace host_pairing

class HostPairingScreenActor {
 public:
  class Delegate {
   public:
    virtual ~Delegate() {}
    virtual void OnActorDestroyed(HostPairingScreenActor* actor) = 0;
  };

  HostPairingScreenActor();
  virtual ~HostPairingScreenActor();

  virtual void Show() = 0;
  virtual void Hide() = 0;
  virtual void SetDelegate(Delegate* delegate) = 0;
  virtual void OnContextChanged(const base::DictionaryValue& diff) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(HostPairingScreenActor);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_HOST_PAIRING_SCREEN_ACTOR_H_
