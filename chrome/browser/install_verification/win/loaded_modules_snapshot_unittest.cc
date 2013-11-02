// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/install_verification/win/loaded_modules_snapshot.h"

#include <Windows.h>

#include <algorithm>
#include <vector>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "chrome/browser/install_verification/win/module_verification_test.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(LoadedModulesSnapshotTest, TestCase) {
  std::vector<HMODULE> snapshot;

  ASSERT_TRUE(GetLoadedModulesSnapshot(&snapshot));
  size_t original_snapshot_size = snapshot.size();
  ASSERT_GT(original_snapshot_size, 0u);
  snapshot.clear();
  HMODULE chrome_dll = ::LoadLibrary(L"chrome.dll");
  ASSERT_NE(static_cast<HMODULE>(NULL), chrome_dll);
  base::ScopedClosureRunner release_chrome_dll(
      base::Bind(base::IgnoreResult(&::FreeLibrary), chrome_dll));
  ASSERT_TRUE(GetLoadedModulesSnapshot(&snapshot));
  ASSERT_GT(snapshot.size(), original_snapshot_size);
  ASSERT_NE(snapshot.end(),
            std::find(snapshot.begin(), snapshot.end(), chrome_dll));
}
