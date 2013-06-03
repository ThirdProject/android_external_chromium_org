// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/indexed_db/indexed_db_factory_impl.h"

#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "content/browser/indexed_db/indexed_db_backing_store.h"
#include "content/browser/indexed_db/indexed_db_database_impl.h"
#include "content/browser/indexed_db/indexed_db_tracing.h"
#include "content/browser/indexed_db/indexed_db_transaction_coordinator.h"
#include "third_party/WebKit/public/platform/WebIDBDatabaseException.h"

namespace content {

template <typename K, typename M>
static void CleanWeakMap(std::map<K, base::WeakPtr<M> >& map) {
  std::map<K, base::WeakPtr<M> > other;
  other.swap(map);

  typename std::map<K, base::WeakPtr<M> >::const_iterator iter = other.begin();
  while (iter != other.end()) {
    if (iter->second.get())
      map[iter->first] = iter->second;
    ++iter;
  }
}

static string16 ComputeFileIdentifier(const string16& database_identifier) {
  string16 suffix(ASCIIToUTF16("@1"));
  string16 result(database_identifier);
  result.insert(result.end(), suffix.begin(), suffix.end());
  return result;
}

static string16 ComputeUniqueIdentifier(const string16& name,
                                        const string16& database_identifier) {
  return ComputeFileIdentifier(database_identifier) + name;
}

IndexedDBFactoryImpl::IndexedDBFactoryImpl() {}

IndexedDBFactoryImpl::~IndexedDBFactoryImpl() {}

void IndexedDBFactoryImpl::RemoveIDBDatabaseBackend(
    const string16& unique_identifier) {
  DCHECK(database_backend_map_.find(unique_identifier) !=
         database_backend_map_.end());
  database_backend_map_.erase(unique_identifier);
}

void IndexedDBFactoryImpl::GetDatabaseNames(
    scoped_refptr<IndexedDBCallbacksWrapper> callbacks,
    const string16& database_identifier,
    const string16& data_directory) {
  IDB_TRACE("IndexedDBFactoryImpl::get_database_names");
  scoped_refptr<IndexedDBBackingStore> backing_store =
      OpenBackingStore(database_identifier, data_directory);
  if (!backing_store) {
    callbacks->OnError(IndexedDBDatabaseError::Create(
        WebKit::WebIDBDatabaseExceptionUnknownError,
        ASCIIToUTF16("Internal error opening backing store for "
                     "indexed_db.webkit_get_database_names.")));
    return;
  }

  callbacks->OnSuccess(backing_store->GetDatabaseNames());
}

void IndexedDBFactoryImpl::DeleteDatabase(
    const string16& name,
    scoped_refptr<IndexedDBCallbacksWrapper> callbacks,
    const string16& database_identifier,
    const string16& data_directory) {
  IDB_TRACE("IndexedDBFactoryImpl::delete_database");
  const string16 unique_identifier =
      ComputeUniqueIdentifier(name, database_identifier);

  IndexedDBDatabaseMap::iterator it =
      database_backend_map_.find(unique_identifier);
  if (it != database_backend_map_.end()) {
    // If there are any connections to the database, directly delete the
    // database.
    it->second->DeleteDatabase(callbacks);
    return;
  }

  // TODO(jsbell): Everything from now on should be done on another thread.
  scoped_refptr<IndexedDBBackingStore> backing_store =
      OpenBackingStore(database_identifier, data_directory);
  if (!backing_store) {
    callbacks->OnError(IndexedDBDatabaseError::Create(
        WebKit::WebIDBDatabaseExceptionUnknownError,
        ASCIIToUTF16("Internal error opening backing store "
                     "for indexed_db.delete_database.")));
    return;
  }

  scoped_refptr<IndexedDBDatabaseImpl> database_backend =
      IndexedDBDatabaseImpl::Create(
          name, backing_store.get(), this, unique_identifier);
  if (!database_backend) {
    callbacks->OnError(IndexedDBDatabaseError::Create(
        WebKit::WebIDBDatabaseExceptionUnknownError,
        ASCIIToUTF16("Internal error creating database backend for "
                     "indexed_db.delete_database.")));
    return;
  }

  database_backend_map_[unique_identifier] = database_backend.get();
  database_backend->DeleteDatabase(callbacks);
  database_backend_map_.erase(unique_identifier);
}

scoped_refptr<IndexedDBBackingStore> IndexedDBFactoryImpl::OpenBackingStore(
    const string16& database_identifier,
    const string16& data_directory) {
  const string16 file_identifier = ComputeFileIdentifier(database_identifier);
  const bool open_in_memory = data_directory.empty();

  IndexedDBBackingStoreMap::iterator it2 =
      backing_store_map_.find(file_identifier);
  if (it2 != backing_store_map_.end() && it2->second.get())
    return it2->second.get();

  scoped_refptr<IndexedDBBackingStore> backing_store;
  if (open_in_memory) {
    backing_store = IndexedDBBackingStore::OpenInMemory(file_identifier);
  } else {
    backing_store = IndexedDBBackingStore::Open(
        database_identifier, data_directory, file_identifier);
  }

  if (backing_store) {
    CleanWeakMap(backing_store_map_);
    backing_store_map_[file_identifier] = backing_store->GetWeakPtr();
    // If an in-memory database, bind lifetime to this factory instance.
    if (open_in_memory)
      session_only_backing_stores_.insert(backing_store);

    // All backing stores associated with this factory should be of the same
    // type.
    DCHECK(session_only_backing_stores_.empty() || open_in_memory);

    return backing_store;
  }

  return 0;
}

void IndexedDBFactoryImpl::Open(
    const string16& name,
    int64 version,
    int64 transaction_id,
    scoped_refptr<IndexedDBCallbacksWrapper> callbacks,
    scoped_refptr<IndexedDBDatabaseCallbacksWrapper> database_callbacks,
    const string16& database_identifier,
    const string16& data_directory) {
  IDB_TRACE("IndexedDBFactoryImpl::open");
  const string16 unique_identifier =
      ComputeUniqueIdentifier(name, database_identifier);

  scoped_refptr<IndexedDBDatabaseImpl> database_backend;
  IndexedDBDatabaseMap::iterator it =
      database_backend_map_.find(unique_identifier);
  if (it == database_backend_map_.end()) {
    scoped_refptr<IndexedDBBackingStore> backing_store =
        OpenBackingStore(database_identifier, data_directory);
    if (!backing_store) {
      callbacks->OnError(IndexedDBDatabaseError::Create(
          WebKit::WebIDBDatabaseExceptionUnknownError,
          ASCIIToUTF16(
              "Internal error opening backing store for indexedDB.open.")));
      return;
    }

    database_backend = IndexedDBDatabaseImpl::Create(
        name, backing_store.get(), this, unique_identifier);
    if (!database_backend) {
      callbacks->OnError(IndexedDBDatabaseError::Create(
          WebKit::WebIDBDatabaseExceptionUnknownError,
          ASCIIToUTF16(
              "Internal error creating database backend for indexedDB.open.")));
      return;
    }

    database_backend_map_[unique_identifier] = database_backend.get();
  } else {
    database_backend = it->second;
  }

  database_backend->OpenConnection(
      callbacks, database_callbacks, transaction_id, version);
}

}  // namespace content
