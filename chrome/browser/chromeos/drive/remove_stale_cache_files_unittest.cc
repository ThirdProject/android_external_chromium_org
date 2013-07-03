// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/file_util.h"
#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/memory/scoped_ptr.h"
#include "base/run_loop.h"
#include "chrome/browser/chromeos/drive/drive.pb.h"
#include "chrome/browser/chromeos/drive/fake_free_disk_space_getter.h"
#include "chrome/browser/chromeos/drive/remove_stale_cache_files.h"
#include "chrome/browser/chromeos/drive/resource_metadata.h"
#include "chrome/browser/chromeos/drive/test_util.h"
#include "chrome/browser/google_apis/test_util.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace drive {
namespace internal {

class RemoveStaleCacheFilesTest : public testing::Test {
 protected:
  virtual void SetUp() OVERRIDE {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());

    fake_free_disk_space_getter_.reset(new FakeFreeDiskSpaceGetter);

    metadata_storage_.reset(new ResourceMetadataStorage(
        temp_dir_.path(), base::MessageLoopProxy::current().get()));

    cache_.reset(new FileCache(metadata_storage_.get(),
                               temp_dir_.path(),
                               base::MessageLoopProxy::current().get(),
                               fake_free_disk_space_getter_.get()));

    resource_metadata_.reset(new ResourceMetadata(
        metadata_storage_.get(), base::MessageLoopProxy::current()));

    ASSERT_TRUE(metadata_storage_->Initialize());
    ASSERT_TRUE(cache_->Initialize());
    ASSERT_EQ(FILE_ERROR_OK, resource_metadata_->Initialize());
  }

  content::TestBrowserThreadBundle thread_bundle_;
  base::ScopedTempDir temp_dir_;

  scoped_ptr<ResourceMetadataStorage,
             test_util::DestroyHelperForTests> metadata_storage_;
  scoped_ptr<FileCache, test_util::DestroyHelperForTests> cache_;
  scoped_ptr<ResourceMetadata, test_util::DestroyHelperForTests>
      resource_metadata_;
  scoped_ptr<FakeFreeDiskSpaceGetter> fake_free_disk_space_getter_;
};

TEST_F(RemoveStaleCacheFilesTest, RemoveStaleCacheFiles) {
  base::FilePath dummy_file;
  ASSERT_TRUE(file_util::CreateTemporaryFileInDir(temp_dir_.path(),
                                                  &dummy_file));
  std::string resource_id("pdf:1a2b3c");
  std::string md5("abcdef0123456789");

  // Create a stale cache file.
  EXPECT_EQ(FILE_ERROR_OK,
            cache_->Store(resource_id, md5, dummy_file,
                          FileCache::FILE_OPERATION_COPY));

  // Verify that the cache entry exists.
  FileCacheEntry cache_entry;
  EXPECT_TRUE(cache_->GetCacheEntry(resource_id, md5, &cache_entry));

  ResourceEntry entry;
  EXPECT_EQ(FILE_ERROR_NOT_FOUND,
            resource_metadata_->GetResourceEntryById(resource_id, &entry));

  // Remove stale cache files.
  RemoveStaleCacheFiles(cache_.get(), resource_metadata_.get());

  // Verify that the cache entry is deleted.
  EXPECT_FALSE(cache_->GetCacheEntry(resource_id, md5, &cache_entry));
}

}  // namespace internal
}  // namespace drive
