// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/screens/host_pairing_screen_actor.h"

namespace chromeos {

namespace host_pairing {

// Keep these constants synced with corresponding constants defined in
// oobe_screen_host_pairing.js.
const char kContextKeyPage[] = "page";
const char kContextKeyDeviceName[] = "deviceName";
const char kContextKeyConfirmationCode[] = "code";
const char kContextKeyEnrollmentDomain[] = "enrollmentDomain";
const char kContextKeyUpdateProgress[] = "updateProgress";

const char kPageWelcome[] = "welcome";
const char kPageCodeConfirmation[] = "code-confirmation";
const char kPageUpdate[] = "update";
const char kPageEnrollmentIntroduction[] = "enrollment-introduction";
const char kPageEnrollment[] = "enrollment";
const char kPageEnrollmentError[] = "enrollment-error";
const char kPagePairingDone[] = "pairing-done";

}  // namespace host_pairing

HostPairingScreenActor::HostPairingScreenActor() {
}

HostPairingScreenActor::~HostPairingScreenActor() {
}

}  // namespace chromeos
