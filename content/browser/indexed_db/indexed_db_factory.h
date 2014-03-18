// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_FACTORY_H_
#define CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_FACTORY_H_

#include <map>
#include <set>

#include "base/basictypes.h"
#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/strings/string16.h"
#include "content/browser/indexed_db/indexed_db_callbacks.h"
#include "content/browser/indexed_db/indexed_db_database.h"
#include "content/browser/indexed_db/indexed_db_database_callbacks.h"
#include "content/common/content_export.h"
#include "url/gurl.h"

namespace content {

class IndexedDBBackingStore;
class IndexedDBContextImpl;
struct IndexedDBPendingConnection;

class CONTENT_EXPORT IndexedDBFactory
    : NON_EXPORTED_BASE(public base::RefCountedThreadSafe<IndexedDBFactory>) {
 public:
  typedef std::multimap<GURL, IndexedDBDatabase*> OriginDBMap;
  typedef OriginDBMap::const_iterator OriginDBMapIterator;

  explicit IndexedDBFactory(IndexedDBContextImpl* context);

  // Notifications from weak pointers.
  void ReleaseDatabase(const IndexedDBDatabase::Identifier& identifier,
                       bool forcedClose);

  void GetDatabaseNames(scoped_refptr<IndexedDBCallbacks> callbacks,
                        const GURL& origin_url,
                        const base::FilePath& data_directory);
  void Open(const base::string16& name,
            const IndexedDBPendingConnection& connection,
            const GURL& origin_url,
            const base::FilePath& data_directory);

  void DeleteDatabase(const base::string16& name,
                      scoped_refptr<IndexedDBCallbacks> callbacks,
                      const GURL& origin_url,
                      const base::FilePath& data_directory);

  void HandleBackingStoreFailure(const GURL& origin_url);

  std::pair<OriginDBMapIterator, OriginDBMapIterator> GetOpenDatabasesForOrigin(
      const GURL& origin_url) const;

  // Called by IndexedDBContext after all connections are closed, to
  // ensure the backing store closed immediately.
  void ForceClose(const GURL& origin_url);

  // Called by the IndexedDBContext destructor so the factory can do cleanup.
  void ContextDestroyed();

  // Called by an IndexedDBDatabase when it is actually deleted.
  void DatabaseDeleted(const IndexedDBDatabase::Identifier& identifier);

  size_t GetConnectionCount(const GURL& origin_url) const;

 protected:
  friend class base::RefCountedThreadSafe<IndexedDBFactory>;

  virtual ~IndexedDBFactory();

  virtual scoped_refptr<IndexedDBBackingStore> OpenBackingStore(
      const GURL& origin_url,
      const base::FilePath& data_directory,
      blink::WebIDBDataLoss* data_loss,
      std::string* data_loss_reason,
      bool* disk_full);

  void ReleaseBackingStore(const GURL& origin_url, bool immediate);
  void CloseBackingStore(const GURL& origin_url);

 private:
  FRIEND_TEST_ALL_PREFIXES(IndexedDBFactoryTest,
                           BackingStoreReleasedOnForcedClose);
  FRIEND_TEST_ALL_PREFIXES(IndexedDBFactoryTest,
                           BackingStoreReleaseDelayedOnClose);
  FRIEND_TEST_ALL_PREFIXES(IndexedDBFactoryTest, DatabaseFailedOpen);
  FRIEND_TEST_ALL_PREFIXES(IndexedDBFactoryTest,
                           DeleteDatabaseClosesBackingStore);
  FRIEND_TEST_ALL_PREFIXES(IndexedDBFactoryTest,
                           ForceCloseReleasesBackingStore);
  FRIEND_TEST_ALL_PREFIXES(IndexedDBFactoryTest,
                           GetDatabaseNamesClosesBackingStore);
  FRIEND_TEST_ALL_PREFIXES(IndexedDBTest,
                           ForceCloseOpenDatabasesOnCommitFailure);

  // Called internally after a database is closed, with some delay. If this
  // factory has the last reference, it will be released.
  void MaybeCloseBackingStore(const GURL& origin_url);
  bool HasLastBackingStoreReference(const GURL& origin_url) const;

  // Testing helpers, so unit tests don't need to grovel through internal state.
  bool IsDatabaseOpen(const GURL& origin_url,
                      const base::string16& name) const;
  bool IsBackingStoreOpen(const GURL& origin_url) const;
  bool IsBackingStorePendingClose(const GURL& origin_url) const;
  void RemoveDatabaseFromMaps(const IndexedDBDatabase::Identifier& identifier);

  IndexedDBContextImpl* context_;

  typedef std::map<IndexedDBDatabase::Identifier,
                   IndexedDBDatabase*> IndexedDBDatabaseMap;
  IndexedDBDatabaseMap database_map_;
  OriginDBMap origin_dbs_;

  typedef std::map<GURL, scoped_refptr<IndexedDBBackingStore> >
      IndexedDBBackingStoreMap;
  IndexedDBBackingStoreMap backing_store_map_;

  std::set<scoped_refptr<IndexedDBBackingStore> > session_only_backing_stores_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_FACTORY_H_
