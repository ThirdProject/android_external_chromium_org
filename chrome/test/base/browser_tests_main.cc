// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>

#include "base/sys_info.h"
#include "chrome/test/base/chrome_test_launcher.h"

int main(int argc, char** argv) {
  int default_jobs = std::max(1, base::SysInfo::NumberOfProcessors() / 2);
  return LaunchChromeTests(default_jobs, argc, argv);
}
