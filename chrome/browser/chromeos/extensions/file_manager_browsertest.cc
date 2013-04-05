// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Browser test for basic Chrome OS file manager functionality:
//  - The file list is updated when a file is added externally to the Downloads
//    folder.
//  - Selecting a file and copy-pasting it with the keyboard copies the file.
//  - Selecting a file and pressing delete deletes it.

#include <algorithm>
#include <string>

#include "base/callback.h"
#include "base/file_util.h"
#include "base/files/file_path.h"
#include "base/files/file_path_watcher.h"
#include "base/platform_file.h"
#include "base/threading/platform_thread.h"
#include "base/time.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/extensions/component_loader.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_test_message_listener.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/extensions/extension.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_view_host.h"
#include "net/base/escape.h"
#include "webkit/fileapi/external_mount_points.h"

namespace {

const char kFileManagerExtensionId[] = "hhaomjibdihmijegdhdafkllkbggdgoj";

const char kKeyboardTestFileName[] = "world.mpeg";
const int64 kKeyboardTestFileSize = 1000;
const char kKeyboardTestFileCopyName[] = "world (1).mpeg";

// The base test class. Used by FileManagerBrowserLocalTest and
// FileManagerBrowserDriveTest.
// TODO(satorux): Add the latter: crbug.com/224534.
class FileManagerBrowserTestBase : public ExtensionApiTest {
 protected:
  // Loads the file manager extension, navigating it to |directory_path| for
  // testing, and waits for it to finish initializing. This is invoked at the
  // start of each test (it crashes if run in SetUp).
  void StartFileManager(const std::string& directory_path);

  // Loads our testing extension and sends it a string identifying the current
  // test.
  void StartTest(const std::string& test_name);
};

void FileManagerBrowserTestBase::StartFileManager(
    const std::string& directory_path) {
  std::string file_manager_url =
      (std::string("chrome-extension://") +
       kFileManagerExtensionId +
       "/main.html#" +
       net::EscapeQueryParamValue(directory_path, false /* use_plus */));

  ui_test_utils::NavigateToURL(browser(), GURL(file_manager_url));

  // This is sent by the file manager when it's finished initializing.
  ExtensionTestMessageListener listener("worker-initialized", false);
  ASSERT_TRUE(listener.WaitUntilSatisfied());
}

void FileManagerBrowserTestBase::StartTest(const std::string& test_name) {
  base::FilePath path = test_data_dir_.AppendASCII("file_manager_browsertest");
  const extensions::Extension* extension = LoadExtensionAsComponent(path);
  ASSERT_TRUE(extension);

  ExtensionTestMessageListener listener("which test", true);
  ASSERT_TRUE(listener.WaitUntilSatisfied());
  listener.Reply(test_name);
}

// The boolean parameter, retrieved by GetParam(), is true if testing in the
// guest mode. See SetUpCommandLine() below for details.
class FileManagerBrowserLocalTest : public FileManagerBrowserTestBase,
                                    public ::testing::WithParamInterface<bool> {
 public:
  virtual void SetUp() OVERRIDE {
    extensions::ComponentLoader::EnableBackgroundExtensionsForTesting();

    ASSERT_TRUE(tmp_dir_.CreateUniqueTempDir());
    downloads_path_ = tmp_dir_.path().Append("Downloads");
    ASSERT_TRUE(file_util::CreateDirectory(downloads_path_));

    CreateTestFile("hello.txt", 123, "4 Sep 1998 12:34:56");
    CreateTestFile("My Desktop Background.png", 1024, "18 Jan 2038 01:02:03");
    CreateTestFile(kKeyboardTestFileName, kKeyboardTestFileSize,
                   "4 July 2012 10:35:00");
    CreateTestDirectory("photos", "1 Jan 1980 23:59:59");
    // Files starting with . are filtered out in
    // file_manager/js/directory_contents.js, so this should not be shown.
    CreateTestDirectory(".warez", "26 Oct 1985 13:39");

    ExtensionApiTest::SetUp();
  }

  virtual void SetUpCommandLine(CommandLine* command_line) OVERRIDE {
    bool in_guest_mode = GetParam();
    if (in_guest_mode) {
      command_line->AppendSwitch(switches::kGuestSession);
      command_line->AppendSwitch(switches::kIncognito);
    }
    ExtensionApiTest::SetUpCommandLine(command_line);
  }

 protected:
  // Creates a file with the given |name|, |length|, and |modification_time|.
  void CreateTestFile(const std::string& name,
                      int length,
                      const std::string& modification_time);

  // Creates an empty directory with the given |name| and |modification_time|.
  void CreateTestDirectory(const std::string& name,
                           const std::string& modification_time);

  // Add a mount point to the fake Downloads directory. Should be called
  // before StartFileManager().
  void AddMountPointToFakeDownloads();

  // Path to the fake Downloads directory used in the test.
  base::FilePath downloads_path_;

 private:
  base::ScopedTempDir tmp_dir_;
};

INSTANTIATE_TEST_CASE_P(InGuestMode,
                        FileManagerBrowserLocalTest,
                        ::testing::Values(true));

INSTANTIATE_TEST_CASE_P(InNonGuestMode,
                        FileManagerBrowserLocalTest,
                        ::testing::Values(false));

void FileManagerBrowserLocalTest::CreateTestFile(
    const std::string& name,
    int length,
    const std::string& modification_time) {
  ASSERT_GE(length, 0);
  base::FilePath path = downloads_path_.AppendASCII(name);
  int flags = base::PLATFORM_FILE_CREATE | base::PLATFORM_FILE_WRITE;
  bool created = false;
  base::PlatformFileError error = base::PLATFORM_FILE_ERROR_FAILED;
  base::PlatformFile file = base::CreatePlatformFile(path, flags,
                                                     &created, &error);
  ASSERT_TRUE(created);
  ASSERT_FALSE(error) << error;
  ASSERT_TRUE(base::TruncatePlatformFile(file, length));
  ASSERT_TRUE(base::ClosePlatformFile(file));
  base::Time time;
  ASSERT_TRUE(base::Time::FromString(modification_time.c_str(), &time));
  ASSERT_TRUE(file_util::SetLastModifiedTime(path, time));
}

void FileManagerBrowserLocalTest::CreateTestDirectory(
    const std::string& name,
    const std::string& modification_time) {
  base::FilePath path = downloads_path_.AppendASCII(name);
  ASSERT_TRUE(file_util::CreateDirectory(path));
  base::Time time;
  ASSERT_TRUE(base::Time::FromString(modification_time.c_str(), &time));
  ASSERT_TRUE(file_util::SetLastModifiedTime(path, time));
}

void FileManagerBrowserLocalTest::AddMountPointToFakeDownloads() {
  // Install our fake Downloads mount point first.
  fileapi::ExternalMountPoints* mount_points =
      content::BrowserContext::GetMountPoints(profile());
  ASSERT_TRUE(mount_points->RevokeFileSystem("Downloads"));
  ASSERT_TRUE(mount_points->RegisterFileSystem(
      "Downloads", fileapi::kFileSystemTypeNativeLocal, downloads_path_));
}

// Monitors changes to a single file until the supplied condition callback
// returns true. Usage:
//   TestFilePathWatcher watcher(path_to_file, MyConditionCallback);
//   watcher.StartAndWaitUntilReady();
//   ... trigger filesystem modification ...
//   watcher.RunMessageLoopUntilConditionSatisfied();
class TestFilePathWatcher {
 public:
  typedef base::Callback<bool(const base::FilePath& file_path)>
      ConditionCallback;

  // Stores the supplied |path| and |condition| for later use (no side effects).
  TestFilePathWatcher(const base::FilePath& path,
                      const ConditionCallback& condition);

  // Waits (running a message pump) until the callback returns true or
  // FilePathWatcher reports an error. Return true on success.
  bool RunMessageLoopUntilConditionSatisfied();

 private:
  // Starts the FilePathWatcher to watch the target file. Also check if the
  // condition is already met.
  void StartWatching();

  // FilePathWatcher callback (on the FILE thread). Posts Done() to the UI
  // thread when the condition is satisfied or there is an error.
  void FilePathWatcherCallback(const base::FilePath& path, bool error);

  const base::FilePath path_;
  ConditionCallback condition_;
  scoped_ptr<base::FilePathWatcher> watcher_;
  base::RunLoop run_loop_;
  base::Closure quit_closure_;
  bool failed_;
};

TestFilePathWatcher::TestFilePathWatcher(const base::FilePath& path,
                                         const ConditionCallback& condition)
    : path_(path),
      condition_(condition),
      quit_closure_(run_loop_.QuitClosure()),
      failed_(false) {
}

void TestFilePathWatcher::StartWatching() {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::FILE));

  watcher_.reset(new base::FilePathWatcher);
  bool ok = watcher_->Watch(
      path_, false /*recursive*/,
      base::Bind(&TestFilePathWatcher::FilePathWatcherCallback,
                 base::Unretained(this)));
  DCHECK(ok);

  // If the condition was already met before FilePathWatcher was launched,
  // FilePathWatcher won't be able to detect a change, so check the condition
  // here.
  if (condition_.Run(path_)) {
    watcher_.reset();
    content::BrowserThread::PostTask(content::BrowserThread::UI,
                                     FROM_HERE,
                                     quit_closure_);
    return;
  }
}

void TestFilePathWatcher::FilePathWatcherCallback(const base::FilePath& path,
                                                  bool failed) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::FILE));
  DCHECK_EQ(path_.value(), path.value());

  if (failed || condition_.Run(path)) {
    failed_ = failed;
    watcher_.reset();
    content::BrowserThread::PostTask(content::BrowserThread::UI,
                                     FROM_HERE,
                                     quit_closure_);
  }
}

bool TestFilePathWatcher::RunMessageLoopUntilConditionSatisfied() {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  content::BrowserThread::PostTask(
      content::BrowserThread::FILE,
      FROM_HERE,
      base::Bind(&TestFilePathWatcher::StartWatching,
                 base::Unretained(this)));

  // Wait until the condition is met.
  run_loop_.Run();
  return !failed_;
}

// Returns true if a file with the given size is present at |path|.
bool FilePresentWithSize(const int64 file_size,
                         const base::FilePath& path) {
  int64 copy_size = 0;
  // If the file doesn't exist yet this will fail and we'll keep waiting.
  if (!file_util::GetFileSize(path, &copy_size))
    return false;
  return (copy_size == file_size);
}

// Returns true if a file is not present at |path|.
bool FileNotPresent(const base::FilePath& path) {
  return !file_util::PathExists(path);
};

IN_PROC_BROWSER_TEST_P(FileManagerBrowserLocalTest, TestFileDisplay) {
  AddMountPointToFakeDownloads();
  StartFileManager("/Downloads");

  ResultCatcher catcher;

  StartTest("file display");

  ExtensionTestMessageListener listener("initial check done", true);
  ASSERT_TRUE(listener.WaitUntilSatisfied());
  CreateTestFile("newly added file.mp3", 2000, "4 Sep 1998 00:00:00");
  listener.Reply("file added");

  ASSERT_TRUE(catcher.GetNextResult()) << catcher.message();
}

IN_PROC_BROWSER_TEST_P(FileManagerBrowserLocalTest, TestKeyboardCopy) {
  AddMountPointToFakeDownloads();
  StartFileManager("/Downloads");

  base::FilePath copy_path =
      downloads_path_.AppendASCII(kKeyboardTestFileCopyName);
  ASSERT_FALSE(file_util::PathExists(copy_path));

  ResultCatcher catcher;
  StartTest("keyboard copy");

  ASSERT_TRUE(catcher.GetNextResult()) << catcher.message();

  TestFilePathWatcher watcher(
      copy_path,
      base::Bind(FilePresentWithSize, kKeyboardTestFileSize));
  ASSERT_TRUE(watcher.RunMessageLoopUntilConditionSatisfied());

  // Check that it was a copy, not a move.
  base::FilePath source_path =
      downloads_path_.AppendASCII(kKeyboardTestFileName);
  ASSERT_TRUE(file_util::PathExists(source_path));
}

IN_PROC_BROWSER_TEST_P(FileManagerBrowserLocalTest, TestKeyboardDelete) {
  AddMountPointToFakeDownloads();
  StartFileManager("/Downloads");

  base::FilePath delete_path =
      downloads_path_.AppendASCII(kKeyboardTestFileName);
  ASSERT_TRUE(file_util::PathExists(delete_path));

  ResultCatcher catcher;
  StartTest("keyboard delete");
  ASSERT_TRUE(catcher.GetNextResult()) << catcher.message();

  TestFilePathWatcher watcher(delete_path,
                              base::Bind(FileNotPresent));
  ASSERT_TRUE(watcher.RunMessageLoopUntilConditionSatisfied());
}

}  // namespace
