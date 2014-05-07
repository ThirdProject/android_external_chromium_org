// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/test_runtime_api_delegate.h"

#include "extensions/common/api/runtime.h"

namespace extensions {

using core_api::runtime::PlatformInfo;

TestRuntimeAPIDelegate::TestRuntimeAPIDelegate() {
}

TestRuntimeAPIDelegate::~TestRuntimeAPIDelegate() {
}

void TestRuntimeAPIDelegate::AddUpdateObserver(UpdateObserver* observer) {
}

void TestRuntimeAPIDelegate::RemoveUpdateObserver(UpdateObserver* observer) {
}

base::Version TestRuntimeAPIDelegate::GetPreviousExtensionVersion(
    const Extension* extension) {
  return base::Version();
}

void TestRuntimeAPIDelegate::ReloadExtension(const std::string& extension_id) {
}

bool TestRuntimeAPIDelegate::CheckForUpdates(
    const std::string& extension_id,
    const UpdateCheckCallback& callback) {
  return false;
}

void TestRuntimeAPIDelegate::OpenURL(const GURL& uninstall_url) {
}

bool TestRuntimeAPIDelegate::GetPlatformInfo(PlatformInfo* info) {
  // TODO(rockot): This probably isn't right. Maybe this delegate should just
  // support manual PlatformInfo override for tests if necessary.
  info->os = PlatformInfo::OS_CROS_;
  return true;
}

bool TestRuntimeAPIDelegate::RestartDevice(std::string* error_message) {
  return false;
}

}  // namespace extensions
