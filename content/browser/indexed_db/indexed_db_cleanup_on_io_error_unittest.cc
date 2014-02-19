// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cerrno>

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "content/browser/indexed_db/indexed_db_backing_store.h"
#include "content/browser/indexed_db/leveldb/leveldb_database.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/leveldatabase/env_chromium.h"

using base::StringPiece;
using content::IndexedDBBackingStore;
using content::LevelDBComparator;
using content::LevelDBDatabase;
using content::LevelDBFactory;
using content::LevelDBSnapshot;

namespace {

class BustedLevelDBDatabase : public LevelDBDatabase {
 public:
  static scoped_ptr<LevelDBDatabase> Open(
      const base::FilePath& file_name,
      const LevelDBComparator* /*comparator*/) {
    return scoped_ptr<LevelDBDatabase>(new BustedLevelDBDatabase);
  }
  virtual bool Get(const base::StringPiece& key,
                   std::string* value,
                   bool* found,
                   const LevelDBSnapshot* = 0) OVERRIDE {
    // false means IO error.
    return false;
  }
};

class MockLevelDBFactory : public LevelDBFactory {
 public:
  MockLevelDBFactory() : destroy_called_(false) {}
  virtual leveldb::Status OpenLevelDB(
      const base::FilePath& file_name,
      const LevelDBComparator* comparator,
      scoped_ptr<LevelDBDatabase>* db,
      bool* is_disk_full = 0) OVERRIDE {
    *db = BustedLevelDBDatabase::Open(file_name, comparator);
    return leveldb::Status::OK();
  }
  virtual bool DestroyLevelDB(const base::FilePath& file_name) OVERRIDE {
    EXPECT_FALSE(destroy_called_);
    destroy_called_ = true;
    return false;
  }
  virtual ~MockLevelDBFactory() { EXPECT_TRUE(destroy_called_); }

 private:
  bool destroy_called_;
};

TEST(IndexedDBIOErrorTest, CleanUpTest) {
  const GURL origin("http://localhost:81");
  base::ScopedTempDir temp_directory;
  ASSERT_TRUE(temp_directory.CreateUniqueTempDir());
  const base::FilePath path = temp_directory.path();
  MockLevelDBFactory mock_leveldb_factory;
  blink::WebIDBDataLoss data_loss =
      blink::WebIDBDataLossNone;
  std::string data_loss_message;
  bool disk_full = false;
  scoped_refptr<IndexedDBBackingStore> backing_store =
      IndexedDBBackingStore::Open(origin,
                                  path,
                                  &data_loss,
                                  &data_loss_message,
                                  &disk_full,
                                  &mock_leveldb_factory);
}

// TODO(dgrogan): Remove expect_destroy if we end up not using it again. It is
// currently set to false in all 4 calls below.
template <class T>
class MockErrorLevelDBFactory : public LevelDBFactory {
 public:
  MockErrorLevelDBFactory(T error, bool expect_destroy)
      : error_(error),
        expect_destroy_(expect_destroy),
        destroy_called_(false) {}
  virtual leveldb::Status OpenLevelDB(
      const base::FilePath& file_name,
      const LevelDBComparator* comparator,
      scoped_ptr<LevelDBDatabase>* db,
      bool* is_disk_full = 0) OVERRIDE {
    return MakeIOError(
        "some filename", "some message", leveldb_env::kNewLogger, error_);
  }
  virtual bool DestroyLevelDB(const base::FilePath& file_name) OVERRIDE {
    EXPECT_FALSE(destroy_called_);
    destroy_called_ = true;
    return false;
  }
  virtual ~MockErrorLevelDBFactory() {
    EXPECT_EQ(expect_destroy_, destroy_called_);
  }

 private:
  T error_;
  bool expect_destroy_;
  bool destroy_called_;
};

TEST(IndexedDBNonRecoverableIOErrorTest, NuancedCleanupTest) {
  const GURL origin("http://localhost:81");
  base::ScopedTempDir temp_directory;
  ASSERT_TRUE(temp_directory.CreateUniqueTempDir());
  const base::FilePath path = temp_directory.path();
  blink::WebIDBDataLoss data_loss =
      blink::WebIDBDataLossNone;
  std::string data_loss_reason;
  bool disk_full = false;

  MockErrorLevelDBFactory<int> mock_leveldb_factory(ENOSPC, false);
  scoped_refptr<IndexedDBBackingStore> backing_store =
      IndexedDBBackingStore::Open(origin,
                                  path,
                                  &data_loss,
                                  &data_loss_reason,
                                  &disk_full,
                                  &mock_leveldb_factory);

  MockErrorLevelDBFactory<base::File::Error> mock_leveldb_factory2(
      base::File::FILE_ERROR_NO_MEMORY, false);
  scoped_refptr<IndexedDBBackingStore> backing_store2 =
      IndexedDBBackingStore::Open(origin,
                                  path,
                                  &data_loss,
                                  &data_loss_reason,
                                  &disk_full,
                                  &mock_leveldb_factory2);

  MockErrorLevelDBFactory<int> mock_leveldb_factory3(EIO, false);
  scoped_refptr<IndexedDBBackingStore> backing_store3 =
      IndexedDBBackingStore::Open(origin,
                                  path,
                                  &data_loss,
                                  &data_loss_reason,
                                  &disk_full,
                                  &mock_leveldb_factory3);

  MockErrorLevelDBFactory<base::File::Error> mock_leveldb_factory4(
      base::File::FILE_ERROR_FAILED, false);
  scoped_refptr<IndexedDBBackingStore> backing_store4 =
      IndexedDBBackingStore::Open(origin,
                                  path,
                                  &data_loss,
                                  &data_loss_reason,
                                  &disk_full,
                                  &mock_leveldb_factory4);
}

}  // namespace
