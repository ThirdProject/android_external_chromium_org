// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_LOGIN_EVENT_RECORDER_H_
#define CHROMEOS_LOGIN_EVENT_RECORDER_H_

#include <string>

#include "base/macros.h"
#include "chromeos/chromeos_export.h"

namespace chromeos {

// LoginEventRecorder records the bootimes of Chrome OS.
// Actual implementation is handled by delegate.
class CHROMEOS_EXPORT LoginEventRecorder {
 public:
  class Delegate {
   public:
    virtual void AddLoginTimeMarker(const std::string& marker_name,
                                    bool send_to_uma) = 0;
  };
  LoginEventRecorder();
  virtual ~LoginEventRecorder();

  static LoginEventRecorder* Get();

  void SetDelegate(Delegate* delegate);

  // Add a time marker for login. A timeline will be dumped to
  // /tmp/login-times-sent after login is done. If |send_to_uma| is true
  // the time between this marker and the last will be sent to UMA with
  // the identifier BootTime.|marker_name|.
  void AddLoginTimeMarker(const std::string& marker_name, bool send_to_uma);

 private:
  Delegate* delegate_;

  DISALLOW_COPY_AND_ASSIGN(LoginEventRecorder);
};

}  // namespace chromeos

#endif  // CHROMEOS_LOGIN_EVENT_RECORDER_H_
