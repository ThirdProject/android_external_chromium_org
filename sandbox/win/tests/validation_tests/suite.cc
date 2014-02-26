// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains the validation tests for the sandbox.
// It includes the tests that need to be performed inside the
// sandbox.

#include <shlwapi.h>

#include "base/win/windows_version.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "sandbox/win/tests/common/controller.h"

#pragma comment(lib, "shlwapi.lib")

namespace {

void TestProcessAccess(sandbox::TestRunner* runner, DWORD target) {
  const wchar_t *kCommandTemplate = L"OpenProcessCmd %d %d";
  wchar_t command[1024] = {0};

  // Test all the scary process permissions.
  wsprintf(command, kCommandTemplate, target, PROCESS_CREATE_THREAD);
  EXPECT_EQ(sandbox::SBOX_TEST_DENIED, runner->RunTest(command));
  wsprintf(command, kCommandTemplate, target, PROCESS_DUP_HANDLE);
  EXPECT_EQ(sandbox::SBOX_TEST_DENIED, runner->RunTest(command));
  wsprintf(command, kCommandTemplate, target, PROCESS_SET_INFORMATION);
  EXPECT_EQ(sandbox::SBOX_TEST_DENIED, runner->RunTest(command));
  wsprintf(command, kCommandTemplate, target, PROCESS_VM_OPERATION);
  EXPECT_EQ(sandbox::SBOX_TEST_DENIED, runner->RunTest(command));
  wsprintf(command, kCommandTemplate, target, PROCESS_VM_READ);
  EXPECT_EQ(sandbox::SBOX_TEST_DENIED, runner->RunTest(command));
  wsprintf(command, kCommandTemplate, target, PROCESS_VM_WRITE);
  EXPECT_EQ(sandbox::SBOX_TEST_DENIED, runner->RunTest(command));
  wsprintf(command, kCommandTemplate, target, PROCESS_QUERY_INFORMATION);
  EXPECT_EQ(sandbox::SBOX_TEST_DENIED, runner->RunTest(command));
  wsprintf(command, kCommandTemplate, target, WRITE_DAC);
  EXPECT_EQ(sandbox::SBOX_TEST_DENIED, runner->RunTest(command));
  wsprintf(command, kCommandTemplate, target, WRITE_OWNER);
  EXPECT_EQ(sandbox::SBOX_TEST_DENIED, runner->RunTest(command));
  wsprintf(command, kCommandTemplate, target, READ_CONTROL);
  EXPECT_EQ(sandbox::SBOX_TEST_DENIED, runner->RunTest(command));
}

}  // namespace

namespace sandbox {

// Returns true if the volume that contains any_path supports ACL security. The
// input path can contain unexpanded environment strings. Returns false on any
// failure or if the file system does not support file security (such as FAT).
bool VolumeSupportsACLs(const wchar_t* any_path) {
  wchar_t expand[MAX_PATH +1];
  DWORD len =::ExpandEnvironmentStringsW(any_path, expand, _countof(expand));
  if (0 == len) return false;
  if (len >  _countof(expand)) return false;
  if (!::PathStripToRootW(expand)) return false;
  DWORD fs_flags = 0;
  if (!::GetVolumeInformationW(expand, NULL, 0, 0, NULL, &fs_flags, NULL, 0))
    return false;
  if (fs_flags & FILE_PERSISTENT_ACLS) return true;
  return false;
}

// Tests if the suite is working properly.
TEST(ValidationSuite, TestSuite) {
  TestRunner runner;
  ASSERT_EQ(SBOX_TEST_PING_OK, runner.RunTest(L"ping"));
}

// Tests if the file system is correctly protected by the sandbox.
TEST(ValidationSuite, TestFileSystem) {
  // Do not perform the test if the system is using FAT or any other
  // file system that does not have file security.
  ASSERT_TRUE(VolumeSupportsACLs(L"%SystemDrive%\\"));
  ASSERT_TRUE(VolumeSupportsACLs(L"%SystemRoot%\\"));
  ASSERT_TRUE(VolumeSupportsACLs(L"%ProgramFiles%\\"));
  ASSERT_TRUE(VolumeSupportsACLs(L"%Temp%\\"));
  ASSERT_TRUE(VolumeSupportsACLs(L"%AppData%\\"));

  TestRunner runner;
  EXPECT_EQ(SBOX_TEST_DENIED, runner.RunTest(L"OpenFile %SystemDrive%"));
  EXPECT_EQ(SBOX_TEST_DENIED, runner.RunTest(L"OpenFile %SystemRoot%"));
  EXPECT_EQ(SBOX_TEST_DENIED, runner.RunTest(L"OpenFile %ProgramFiles%"));
  EXPECT_EQ(SBOX_TEST_DENIED,
      runner.RunTest(L"OpenFile %SystemRoot%\\System32"));
  EXPECT_EQ(SBOX_TEST_DENIED,
      runner.RunTest(L"OpenFile %SystemRoot%\\explorer.exe"));
  EXPECT_EQ(SBOX_TEST_DENIED,
      runner.RunTest(L"OpenFile %SystemRoot%\\Cursors\\arrow_i.cur"));
  EXPECT_EQ(SBOX_TEST_DENIED,
      runner.RunTest(L"OpenFile %AllUsersProfile%"));
  EXPECT_EQ(SBOX_TEST_DENIED, runner.RunTest(L"OpenFile %Temp%"));
  EXPECT_EQ(SBOX_TEST_DENIED, runner.RunTest(L"OpenFile %AppData%"));
}

// Tests if the registry is correctly protected by the sandbox.
TEST(ValidationSuite, TestRegistry) {
  TestRunner runner;
  EXPECT_EQ(SBOX_TEST_DENIED, runner.RunTest(L"OpenKey HKLM"));
  EXPECT_EQ(SBOX_TEST_DENIED, runner.RunTest(L"OpenKey HKCU"));
  EXPECT_EQ(SBOX_TEST_DENIED, runner.RunTest(L"OpenKey HKU"));
  EXPECT_EQ(SBOX_TEST_DENIED,
      runner.RunTest(
          L"OpenKey HKLM "
          L"\"Software\\Microsoft\\Windows NT\\CurrentVersion\\WinLogon\""));
}

// Tests that the permissions on the Windowstation does not allow the sandbox
// to get to the interactive desktop or to make the sbox desktop interactive.
TEST(ValidationSuite, TestDesktop) {
  TestRunner runner;
  runner.GetPolicy()->SetAlternateDesktop(false);
  EXPECT_EQ(SBOX_TEST_DENIED, runner.RunTest(L"OpenInteractiveDesktop NULL"));
  EXPECT_EQ(SBOX_TEST_DENIED, runner.RunTest(L"SwitchToSboxDesktop NULL"));
}

// Tests if the windows are correctly protected by the sandbox.
TEST(ValidationSuite, TestWindows) {
  TestRunner runner;
  wchar_t command[1024] = {0};

  wsprintf(command, L"ValidWindow %d", ::GetDesktopWindow());
  EXPECT_EQ(SBOX_TEST_DENIED, runner.RunTest(command));

  wsprintf(command, L"ValidWindow %d", ::FindWindow(NULL, NULL));
  EXPECT_EQ(SBOX_TEST_DENIED, runner.RunTest(command));
}

// Tests that a locked-down process cannot open another locked-down process.
TEST(ValidationSuite, TestProcessDenyLockdown) {
  TestRunner runner;
  TestRunner target;
  wchar_t command[1024] = {0};

  target.SetAsynchronous(true);

  EXPECT_EQ(SBOX_TEST_SUCCEEDED, target.RunTest(L"SleepCmd 30000"));

  TestProcessAccess(&runner, target.process_id());
}

// Tests that a low-integrity process cannot open a locked-down process (due
// to the integrity label changing after startup via SetDelayedIntegrityLevel).
TEST(ValidationSuite, TestProcessDenyLowIntegrity) {
  // This test applies only to Vista and above.
  if (base::win::Version() < base::win::VERSION_VISTA)
    return;

  TestRunner runner;
  TestRunner target;
  wchar_t command[1024] = {0};

  target.SetAsynchronous(true);
  target.GetPolicy()->SetDelayedIntegrityLevel(INTEGRITY_LEVEL_LOW);

  runner.GetPolicy()->SetIntegrityLevel(INTEGRITY_LEVEL_LOW);
  runner.GetPolicy()->SetTokenLevel(USER_RESTRICTED_SAME_ACCESS,
                                    USER_INTERACTIVE);

  EXPECT_EQ(SBOX_TEST_SUCCEEDED, target.RunTest(L"SleepCmd 30000"));

  TestProcessAccess(&runner, target.process_id());
}

// Tests that a locked-down process cannot open a low-integrity process.
TEST(ValidationSuite, TestProcessDenyBelowLowIntegrity) {
  //  This test applies only to Vista and above.
  if (base::win::Version() < base::win::VERSION_VISTA)
    return;

  TestRunner runner;
  TestRunner target;
  wchar_t command[1024] = {0};

  target.SetAsynchronous(true);
  target.GetPolicy()->SetIntegrityLevel(INTEGRITY_LEVEL_LOW);
  target.GetPolicy()->SetTokenLevel(USER_RESTRICTED_SAME_ACCESS,
                                    USER_INTERACTIVE);

  runner.GetPolicy()->SetDelayedIntegrityLevel(INTEGRITY_LEVEL_UNTRUSTED);
  runner.GetPolicy()->SetTokenLevel(USER_RESTRICTED_SAME_ACCESS,
                                    USER_INTERACTIVE);

  EXPECT_EQ(SBOX_TEST_SUCCEEDED, target.RunTest(L"SleepCmd 30000"));

  TestProcessAccess(&runner, target.process_id());
}

// Tests if the threads are correctly protected by the sandbox.
TEST(ValidationSuite, TestThread) {
  TestRunner runner;
  wchar_t command[1024] = {0};

  wsprintf(command, L"OpenThreadCmd %d", ::GetCurrentThreadId());
  EXPECT_EQ(SBOX_TEST_DENIED, runner.RunTest(command));
}

}  // namespace sandbox
