// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/fileapi/syncable/local_file_sync_context.h"

#include <vector>

#include "base/bind.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/message_loop.h"
#include "base/platform_file.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "webkit/blob/mock_blob_url_request_context.h"
#include "webkit/fileapi/file_system_context.h"
#include "webkit/fileapi/file_system_operation.h"
#include "webkit/fileapi/isolated_context.h"
#include "webkit/fileapi/syncable/canned_syncable_file_system.h"
#include "webkit/fileapi/syncable/local_file_change_tracker.h"
#include "webkit/fileapi/syncable/sync_status_code.h"
#include "webkit/fileapi/syncable/syncable_file_system_util.h"

#define FPL FILE_PATH_LITERAL

// This tests LocalFileSyncContext behavior in multi-thread /
// multi-file-system-context environment.
// Basic combined tests (single-thread / single-file-system-context)
// that involve LocalFileSyncContext are also in
// syncable_file_system_unittests.cc.

namespace fileapi {

namespace {
const char kOrigin1[] = "http://example.com";
const char kOrigin2[] = "http://chromium.org";
const char kServiceName[] = "test";
}

class LocalFileSyncContextTest : public testing::Test {
 protected:
  LocalFileSyncContextTest()
      : status_(SYNC_FILE_ERROR_FAILED),
        file_error_(base::PLATFORM_FILE_ERROR_FAILED),
        async_modify_finished_(false),
        has_inflight_prepare_for_sync_(false) {}

  virtual void SetUp() OVERRIDE {
    EXPECT_TRUE(fileapi::RegisterSyncableFileSystem(kServiceName));

    io_thread_.reset(new base::Thread("Thread_IO"));
    io_thread_->StartWithOptions(
        base::Thread::Options(MessageLoop::TYPE_IO, 0));

    ui_task_runner_ = MessageLoop::current()->message_loop_proxy();
    io_task_runner_ = io_thread_->message_loop_proxy();
  }

  virtual void TearDown() OVERRIDE {
    EXPECT_TRUE(fileapi::RevokeSyncableFileSystem(kServiceName));
    io_thread_->Stop();
  }

  void StartPrepareForSync(LocalFileSyncContext* sync_context,
                           FileSystemContext* file_system_context,
                           const FileSystemURL& url,
                           FileChangeList* changes) {
    ASSERT_TRUE(changes != NULL);
    ASSERT_FALSE(has_inflight_prepare_for_sync_);
    status_ = SYNC_STATUS_UNKNOWN;
    has_inflight_prepare_for_sync_ = true;
    sync_context->PrepareForSync(
        file_system_context,
        url,
        base::Bind(&LocalFileSyncContextTest::DidPrepareForSync,
                   base::Unretained(this), changes));
  }

  SyncStatusCode PrepareForSync(LocalFileSyncContext* sync_context,
                                FileSystemContext* file_system_context,
                                const FileSystemURL& url,
                                FileChangeList* changes) {
    StartPrepareForSync(sync_context, file_system_context, url, changes);
    MessageLoop::current()->Run();
    return status_;
  }

  base::Closure GetPrepareForSyncClosure(LocalFileSyncContext* sync_context,
                                         FileSystemContext* file_system_context,
                                         const FileSystemURL& url,
                                         FileChangeList* changes) {
    return base::Bind(&LocalFileSyncContextTest::StartPrepareForSync,
                      base::Unretained(this), base::Unretained(sync_context),
                      base::Unretained(file_system_context),
                      url, changes);
  }

  void DidPrepareForSync(FileChangeList* changes_out,
                         SyncStatusCode status,
                         const FileChangeList& changes) {
    ASSERT_TRUE(ui_task_runner_->RunsTasksOnCurrentThread());
    has_inflight_prepare_for_sync_ = false;
    status_ = status;
    *changes_out = changes;
    MessageLoop::current()->Quit();
  }

  SyncStatusCode ApplyRemoteChange(FileSystemContext* file_system_context,
                                   const FileChange& change,
                                   const FilePath& local_path,
                                   const FileSystemURL& url) {
    status_ = SYNC_STATUS_UNKNOWN;

    // First we should call PrepareForSync to disable writing.
    FileChangeList changes;
    StartPrepareForSync(sync_context_, file_system_context, url, &changes);
    MessageLoop::current()->Run();
    EXPECT_EQ(SYNC_STATUS_OK, status_);

    sync_context_->ApplyRemoteChange(
        file_system_context, change, local_path, url,
        base::Bind(&LocalFileSyncContextTest::DidApplyRemoteChange,
                   base::Unretained(this)));
    MessageLoop::current()->Run();
    return status_;
  }

  void DidApplyRemoteChange(SyncStatusCode status) {
    MessageLoop::current()->Quit();
    status_ = status;
  }

  void StartModifyFileOnIOThread(CannedSyncableFileSystem* file_system,
                                 const FileSystemURL& url) {
    async_modify_finished_ = false;
    ASSERT_TRUE(file_system != NULL);
    if (!io_task_runner_->RunsTasksOnCurrentThread()) {
      ASSERT_TRUE(ui_task_runner_->RunsTasksOnCurrentThread());
      io_task_runner_->PostTask(
          FROM_HERE,
          base::Bind(&LocalFileSyncContextTest::StartModifyFileOnIOThread,
                     base::Unretained(this), file_system, url));
      return;
    }
    ASSERT_TRUE(io_task_runner_->RunsTasksOnCurrentThread());
    file_error_ = base::PLATFORM_FILE_ERROR_FAILED;
    file_system->NewOperation()->Truncate(
        url, 1, base::Bind(&LocalFileSyncContextTest::DidModifyFile,
                           base::Unretained(this)));
  }

  base::PlatformFileError WaitUntilModifyFileIsDone() {
    while (!async_modify_finished_)
      MessageLoop::current()->RunAllPending();
    return file_error_;
  }

  void DidModifyFile(base::PlatformFileError error) {
    if (!ui_task_runner_->RunsTasksOnCurrentThread()) {
      ASSERT_TRUE(io_task_runner_->RunsTasksOnCurrentThread());
      ui_task_runner_->PostTask(
          FROM_HERE,
          base::Bind(&LocalFileSyncContextTest::DidModifyFile,
                     base::Unretained(this), error));
      return;
    }
    ASSERT_TRUE(ui_task_runner_->RunsTasksOnCurrentThread());
    file_error_ = error;
    async_modify_finished_ = true;
  }

  // These need to remain until the very end.
  scoped_ptr<base::Thread> io_thread_;
  MessageLoop loop_;

  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner_;

  scoped_refptr<LocalFileSyncContext> sync_context_;

  SyncStatusCode status_;
  base::PlatformFileError file_error_;
  bool async_modify_finished_;
  bool has_inflight_prepare_for_sync_;
};

TEST_F(LocalFileSyncContextTest, ConstructAndDestruct) {
  sync_context_ = new LocalFileSyncContext(
      ui_task_runner_, io_task_runner_);
  sync_context_->ShutdownOnUIThread();
}

TEST_F(LocalFileSyncContextTest, InitializeFileSystemContext) {
  CannedSyncableFileSystem file_system(GURL(kOrigin1), kServiceName,
                                       io_task_runner_);
  file_system.SetUp();

  sync_context_ = new LocalFileSyncContext(ui_task_runner_, io_task_runner_);

  // Initializes file_system using |sync_context_|.
  EXPECT_EQ(SYNC_STATUS_OK,
            file_system.MaybeInitializeFileSystemContext(sync_context_));

  // Make sure everything's set up for file_system to be able to handle
  // syncable file system operations.
  EXPECT_TRUE(file_system.file_system_context()->sync_context() != NULL);
  EXPECT_TRUE(file_system.file_system_context()->change_tracker() != NULL);
  EXPECT_EQ(sync_context_.get(),
            file_system.file_system_context()->sync_context());

  // Calling MaybeInitialize for the same context multiple times must be ok.
  EXPECT_EQ(SYNC_STATUS_OK,
            file_system.MaybeInitializeFileSystemContext(sync_context_));
  EXPECT_EQ(sync_context_.get(),
            file_system.file_system_context()->sync_context());

  // Opens the file_system, perform some operation and see if the change tracker
  // correctly captures the change.
  EXPECT_EQ(base::PLATFORM_FILE_OK, file_system.OpenFileSystem());

  const FileSystemURL kURL(file_system.URL("foo"));
  EXPECT_EQ(base::PLATFORM_FILE_OK, file_system.CreateFile(kURL));

  std::vector<FileSystemURL> urls;
  file_system.file_system_context()->change_tracker()->GetChangedURLs(&urls);
  ASSERT_EQ(1U, urls.size());
  EXPECT_EQ(kURL, urls[0]);

  // Finishing the test.
  sync_context_->ShutdownOnUIThread();
  file_system.TearDown();
}

TEST_F(LocalFileSyncContextTest, MultipleFileSystemContexts) {
  CannedSyncableFileSystem file_system1(GURL(kOrigin1), kServiceName,
                                        io_task_runner_);
  CannedSyncableFileSystem file_system2(GURL(kOrigin2), kServiceName,
                                        io_task_runner_);
  file_system1.SetUp();
  file_system2.SetUp();

  sync_context_ = new LocalFileSyncContext(ui_task_runner_, io_task_runner_);

  // Initializes file_system1 and file_system2.
  EXPECT_EQ(SYNC_STATUS_OK,
            file_system1.MaybeInitializeFileSystemContext(sync_context_));
  EXPECT_EQ(SYNC_STATUS_OK,
            file_system2.MaybeInitializeFileSystemContext(sync_context_));

  EXPECT_EQ(base::PLATFORM_FILE_OK, file_system1.OpenFileSystem());
  EXPECT_EQ(base::PLATFORM_FILE_OK, file_system2.OpenFileSystem());

  const FileSystemURL kURL1(file_system1.URL("foo"));
  const FileSystemURL kURL2(file_system2.URL("bar"));

  // Creates a file in file_system1.
  EXPECT_EQ(base::PLATFORM_FILE_OK, file_system1.CreateFile(kURL1));

  // file_system1's tracker must have recorded the change.
  std::vector<FileSystemURL> urls;
  file_system1.file_system_context()->change_tracker()->GetChangedURLs(&urls);
  ASSERT_EQ(1U, urls.size());
  EXPECT_EQ(kURL1, urls[0]);

  // file_system1's tracker must have no change.
  urls.clear();
  file_system2.file_system_context()->change_tracker()->GetChangedURLs(&urls);
  ASSERT_TRUE(urls.empty());

  // Creates a directory in file_system2.
  EXPECT_EQ(base::PLATFORM_FILE_OK, file_system2.CreateDirectory(kURL2));

  // file_system1's tracker must have the change for kURL1 as before.
  urls.clear();
  file_system1.file_system_context()->change_tracker()->GetChangedURLs(&urls);
  ASSERT_EQ(1U, urls.size());
  EXPECT_EQ(kURL1, urls[0]);

  // file_system2's tracker now must have the change for kURL2.
  urls.clear();
  file_system2.file_system_context()->change_tracker()->GetChangedURLs(&urls);
  ASSERT_EQ(1U, urls.size());
  EXPECT_EQ(kURL2, urls[0]);

  FileChangeList changes;
  EXPECT_EQ(SYNC_STATUS_OK, PrepareForSync(sync_context_,
                                           file_system1.file_system_context(),
                                           kURL1, &changes));
  EXPECT_EQ(1U, changes.size());
  EXPECT_TRUE(changes.list().back().IsFile());
  EXPECT_TRUE(changes.list().back().IsAddOrUpdate());

  changes.clear();
  EXPECT_EQ(SYNC_STATUS_OK, PrepareForSync(sync_context_,
                                           file_system2.file_system_context(),
                                           kURL2, &changes));
  EXPECT_EQ(1U, changes.size());
  EXPECT_FALSE(changes.list().back().IsFile());
  EXPECT_TRUE(changes.list().back().IsAddOrUpdate());

  sync_context_->ShutdownOnUIThread();
  sync_context_ = NULL;

  file_system1.TearDown();
  file_system2.TearDown();
}

TEST_F(LocalFileSyncContextTest, PrepareSyncWhileWriting) {
  CannedSyncableFileSystem file_system(GURL(kOrigin1), kServiceName,
                                       io_task_runner_);
  file_system.SetUp();
  sync_context_ = new LocalFileSyncContext(ui_task_runner_, io_task_runner_);
  EXPECT_EQ(SYNC_STATUS_OK,
            file_system.MaybeInitializeFileSystemContext(sync_context_));

  EXPECT_EQ(base::PLATFORM_FILE_OK, file_system.OpenFileSystem());

  const FileSystemURL kURL1(file_system.URL("foo"));

  // Creates a file in file_system.
  EXPECT_EQ(base::PLATFORM_FILE_OK, file_system.CreateFile(kURL1));

  // Kick file write on IO thread.
  StartModifyFileOnIOThread(&file_system, kURL1);

  // Until the operation finishes PrepareForSync should return BUSY error.
  FileChangeList changes;
  EXPECT_EQ(SYNC_STATUS_FILE_BUSY,
            PrepareForSync(sync_context_, file_system.file_system_context(),
                           kURL1, &changes));
  EXPECT_TRUE(changes.empty());

  // Register PrepareForSync method to be invoked when kURL1 becomes
  // syncable. (Actually this may be done after all operations are done
  // on IO thread in this test.)
  sync_context_->RegisterURLForWaitingSync(
      kURL1, GetPrepareForSyncClosure(sync_context_,
                                      file_system.file_system_context(),
                                      kURL1, &changes));

  // Wait for the completion.
  EXPECT_EQ(base::PLATFORM_FILE_OK, WaitUntilModifyFileIsDone());

  // The PrepareForSync must have been started; wait until DidPrepareForSync
  // is done.
  MessageLoop::current()->Run();
  ASSERT_FALSE(has_inflight_prepare_for_sync_);

  // Now PrepareForSync should have run and returned OK.
  EXPECT_EQ(SYNC_STATUS_OK, status_);
  EXPECT_EQ(1U, changes.size());
  EXPECT_TRUE(changes.list().back().IsFile());
  EXPECT_TRUE(changes.list().back().IsAddOrUpdate());

  sync_context_->ShutdownOnUIThread();
  sync_context_ = NULL;
  file_system.TearDown();
}

TEST_F(LocalFileSyncContextTest, ApplyRemoteChangeForDeletion) {
  CannedSyncableFileSystem file_system(GURL(kOrigin1), kServiceName,
                                       io_task_runner_);
  file_system.SetUp();

  sync_context_ = new LocalFileSyncContext(ui_task_runner_, io_task_runner_);
  ASSERT_EQ(SYNC_STATUS_OK,
            file_system.MaybeInitializeFileSystemContext(sync_context_));
  ASSERT_EQ(base::PLATFORM_FILE_OK, file_system.OpenFileSystem());

  // Record the initial usage (likely 0).
  int64 initial_usage = -1;
  int64 quota = -1;
  EXPECT_EQ(quota::kQuotaStatusOk,
            file_system.GetUsageAndQuota(&initial_usage, &quota));

  // Create a file and directory in the file_system.
  const FileSystemURL kFile(file_system.URL("file"));
  const FileSystemURL kDir(file_system.URL("dir"));
  const FileSystemURL kChild(file_system.URL("dir/child"));

  EXPECT_EQ(base::PLATFORM_FILE_OK, file_system.CreateFile(kFile));
  EXPECT_EQ(base::PLATFORM_FILE_OK, file_system.CreateDirectory(kDir));
  EXPECT_EQ(base::PLATFORM_FILE_OK, file_system.CreateFile(kChild));

  // file_system's change tracker must have recorded the creation.
  std::vector<FileSystemURL> urls;
  file_system.file_system_context()->change_tracker()->GetChangedURLs(&urls);
  ASSERT_EQ(3U, urls.size());
  for (size_t i = 0; i < urls.size(); ++i) {
    EXPECT_TRUE(urls[i] == kFile || urls[i] == kDir || urls[i] == kChild);
    file_system.file_system_context()->change_tracker()->FinalizeSyncForURL(
        urls[i]);
  }

  // At this point the usage must be greater than the initial usage.
  int64 new_usage = -1;
  EXPECT_EQ(quota::kQuotaStatusOk,
            file_system.GetUsageAndQuota(&new_usage, &quota));
  EXPECT_GT(new_usage, initial_usage);

  // Now let's apply remote deletion changes.
  FileChange change(FileChange::FILE_CHANGE_DELETE,
                    FileChange::FILE_TYPE_FILE);
  EXPECT_EQ(SYNC_STATUS_OK,
            ApplyRemoteChange(file_system.file_system_context(),
                              change, FilePath(), kFile));

  // The implementation doesn't check file type for deletion, and it must be ok
  // even if we don't know if the deletion change was for a file or a directory.
  change = FileChange(FileChange::FILE_CHANGE_DELETE,
                      FileChange::FILE_TYPE_UNDETERMINED);
  EXPECT_EQ(SYNC_STATUS_OK,
            ApplyRemoteChange(file_system.file_system_context(),
                              change, FilePath(), kDir));

  // Check the directory/files are deleted successfully.
  EXPECT_EQ(base::PLATFORM_FILE_ERROR_NOT_FOUND,
            file_system.FileExists(kFile));
  EXPECT_EQ(base::PLATFORM_FILE_ERROR_NOT_FOUND,
            file_system.DirectoryExists(kDir));
  EXPECT_EQ(base::PLATFORM_FILE_ERROR_NOT_FOUND,
            file_system.FileExists(kChild));

  // The changes applied by ApplyRemoteChange should not be recorded in
  // the change tracker.
  urls.clear();
  file_system.file_system_context()->change_tracker()->GetChangedURLs(&urls);
  EXPECT_TRUE(urls.empty());

  // The quota usage data must have reflected the deletion.
  EXPECT_EQ(quota::kQuotaStatusOk,
            file_system.GetUsageAndQuota(&new_usage, &quota));
  EXPECT_EQ(new_usage, initial_usage);

  sync_context_->ShutdownOnUIThread();
  sync_context_ = NULL;
  file_system.TearDown();
}

TEST_F(LocalFileSyncContextTest, ApplyRemoteChangeForAddOrUpdate) {
  ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  CannedSyncableFileSystem file_system(GURL(kOrigin1), kServiceName,
                                       io_task_runner_);
  file_system.SetUp();

  sync_context_ = new LocalFileSyncContext(ui_task_runner_, io_task_runner_);
  ASSERT_EQ(SYNC_STATUS_OK,
            file_system.MaybeInitializeFileSystemContext(sync_context_));
  ASSERT_EQ(base::PLATFORM_FILE_OK, file_system.OpenFileSystem());

  const FileSystemURL kFile1(file_system.URL("file1"));
  const FileSystemURL kFile2(file_system.URL("file2"));
  const FileSystemURL kDir(file_system.URL("dir"));

  const char kTestFileData0[] = "0123456789";
  const char kTestFileData1[] = "Lorem ipsum!";
  const char kTestFileData2[] = "This is sample test data.";

  // Create kFile1 and populate it with kTestFileData0.
  EXPECT_EQ(base::PLATFORM_FILE_OK, file_system.CreateFile(kFile1));
  EXPECT_EQ(static_cast<int64>(arraysize(kTestFileData0) - 1),
            file_system.WriteString(kFile1, kTestFileData0));

  // kFile2 and kDir are not there yet.
  EXPECT_EQ(base::PLATFORM_FILE_ERROR_NOT_FOUND,
            file_system.FileExists(kFile2));
  EXPECT_EQ(base::PLATFORM_FILE_ERROR_NOT_FOUND,
            file_system.DirectoryExists(kDir));

  // file_system's change tracker must have recorded the creation.
  std::vector<FileSystemURL> urls;
  file_system.file_system_context()->change_tracker()->GetChangedURLs(&urls);
  ASSERT_EQ(1U, urls.size());
  EXPECT_EQ(kFile1, urls[0]);
  file_system.file_system_context()->change_tracker()->FinalizeSyncForURL(
      urls[0]);

  // Prepare temporary files which represent the remote file data.
  const FilePath kFilePath1(temp_dir.path().Append(FPL("file1")));
  const FilePath kFilePath2(temp_dir.path().Append(FPL("file2")));

  ASSERT_EQ(static_cast<int>(arraysize(kTestFileData1) - 1),
            file_util::WriteFile(kFilePath1, kTestFileData1,
                                 arraysize(kTestFileData1) - 1));
  ASSERT_EQ(static_cast<int>(arraysize(kTestFileData2) - 1),
            file_util::WriteFile(kFilePath2, kTestFileData2,
                                 arraysize(kTestFileData2) - 1));

  // Record the usage.
  int64 usage = -1, new_usage = -1;
  int64 quota = -1;
  EXPECT_EQ(quota::kQuotaStatusOk,
            file_system.GetUsageAndQuota(&usage, &quota));

  // Here in the local filesystem we have:
  //  * kFile1 with kTestFileData0
  //
  // In the remote side let's assume we have:
  //  * kFile1 with kTestFileData1
  //  * kFile2 with kTestFileData2
  //  * kDir
  //
  // By calling ApplyChange's:
  //  * kFile1 will be updated to have kTestFileData1
  //  * kFile2 will be created
  //  * kDir will be created

  // Apply the remote change to kFile1 (which will update the file).
  FileChange change(FileChange::FILE_CHANGE_ADD_OR_UPDATE,
                    FileChange::FILE_TYPE_FILE);
  EXPECT_EQ(SYNC_STATUS_OK,
            ApplyRemoteChange(file_system.file_system_context(),
                              change, kFilePath1, kFile1));

  // Check if the usage has been increased by (kTestFileData1 - kTestFileData0).
  const int updated_size =
      arraysize(kTestFileData1) - arraysize(kTestFileData0);
  EXPECT_EQ(quota::kQuotaStatusOk,
            file_system.GetUsageAndQuota(&new_usage, &quota));
  EXPECT_EQ(updated_size, new_usage - usage);

  // Apply remote changes to kFile2 and kDir (should create a file and
  // directory respectively).
  change = FileChange(FileChange::FILE_CHANGE_ADD_OR_UPDATE,
                      FileChange::FILE_TYPE_FILE);
  EXPECT_EQ(SYNC_STATUS_OK,
            ApplyRemoteChange(file_system.file_system_context(),
                              change, kFilePath2, kFile2));

  change = FileChange(FileChange::FILE_CHANGE_ADD_OR_UPDATE,
                      FileChange::FILE_TYPE_DIRECTORY);
  EXPECT_EQ(SYNC_STATUS_OK,
            ApplyRemoteChange(file_system.file_system_context(),
                              change, FilePath(), kDir));

  // This should not happen, but calling ApplyRemoteChange
  // with wrong file type will result in error.
  change = FileChange(FileChange::FILE_CHANGE_ADD_OR_UPDATE,
                      FileChange::FILE_TYPE_FILE);
  EXPECT_EQ(SYNC_FILE_ERROR_FAILED,
            ApplyRemoteChange(file_system.file_system_context(),
                              change, kFilePath1, kDir));

  // Creating a file/directory must have increased the usage more than
  // the size of kTestFileData2.
  new_usage = usage;
  EXPECT_EQ(quota::kQuotaStatusOk,
            file_system.GetUsageAndQuota(&new_usage, &quota));
  EXPECT_GT(new_usage,
            static_cast<int64>(usage + arraysize(kTestFileData2) - 1));

  // The changes applied by ApplyRemoteChange should not be recorded in
  // the change tracker.
  urls.clear();
  file_system.file_system_context()->change_tracker()->GetChangedURLs(&urls);
  EXPECT_TRUE(urls.empty());

  // Make sure all three files/directory exist.
  EXPECT_EQ(base::PLATFORM_FILE_OK, file_system.FileExists(kFile1));
  EXPECT_EQ(base::PLATFORM_FILE_OK, file_system.FileExists(kFile2));
  EXPECT_EQ(base::PLATFORM_FILE_OK, file_system.DirectoryExists(kDir));

  sync_context_->ShutdownOnUIThread();
  file_system.TearDown();
}

}  // namespace fileapi
