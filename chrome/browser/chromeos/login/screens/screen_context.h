// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_SCREEN_CONTEXT_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_SCREEN_CONTEXT_H_

#include <string>

namespace chromeos {

// Placeholder for class that will be used to easily share values between JS and
// C++ screens.
class ScreenContext {
 public:
  ScreenContext() {}
  ~ScreenContext() {}
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_SCREEN_CONTEXT_H_
