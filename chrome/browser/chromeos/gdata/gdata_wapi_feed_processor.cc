// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/metrics/histogram.h"
#include "chrome/browser/chromeos/gdata/gdata_directory_service.h"
#include "chrome/browser/chromeos/gdata/gdata_files.h"
#include "chrome/browser/chromeos/gdata/gdata_wapi_feed_processor.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

namespace gdata {

namespace {

// Recursively extracts the paths set of all sub-directories of |entry|.
void GetChildDirectoryPaths(GDataEntry* entry,
                            std::set<FilePath>* changed_dirs) {
  GDataDirectory* dir = entry->AsGDataDirectory();
  if (!dir)
    return;

  for (GDataDirectoryCollection::const_iterator it =
       dir->child_directories().begin();
       it != dir->child_directories().end(); ++it) {
    GDataDirectory* child_dir = it->second;
    changed_dirs->insert(child_dir->GetFilePath());
    GetChildDirectoryPaths(child_dir, changed_dirs);
  }
}

}  // namespace

FeedToFileResourceMapUmaStats::FeedToFileResourceMapUmaStats()
    : num_regular_files(0),
      num_hosted_documents(0) {
}

FeedToFileResourceMapUmaStats::~FeedToFileResourceMapUmaStats() {
}

GDataWapiFeedProcessor::GDataWapiFeedProcessor(
    GDataDirectoryService* directory_service)
  : directory_service_(directory_service) {
}

GDataWapiFeedProcessor::~GDataWapiFeedProcessor() {
}

GDataFileError GDataWapiFeedProcessor::ApplyFeeds(
    const std::vector<DocumentFeed*>& feed_list,
    int64 start_changestamp,
    int64 root_feed_changestamp,
    std::set<FilePath>* changed_dirs) {
  bool is_delta_feed = start_changestamp != 0;

  directory_service_->set_origin(FROM_SERVER);

  int64 delta_feed_changestamp = 0;
  FeedToFileResourceMapUmaStats uma_stats;
  FileResourceIdMap file_map;
  GDataFileError error = FeedToFileResourceMap(feed_list,
                                               &file_map,
                                               &delta_feed_changestamp,
                                               &uma_stats);
  if (error != GDATA_FILE_OK)
    return error;

  ApplyFeedFromFileUrlMap(
      is_delta_feed,
      is_delta_feed ? delta_feed_changestamp : root_feed_changestamp,
      &file_map,
      changed_dirs);

  // Shouldn't record histograms when processing delta feeds.
  if (!is_delta_feed)
    UpdateFileCountUmaHistograms(uma_stats);

  return GDATA_FILE_OK;
}

void GDataWapiFeedProcessor::UpdateFileCountUmaHistograms(
    const FeedToFileResourceMapUmaStats& uma_stats) const {
  const int num_total_files =
      uma_stats.num_hosted_documents + uma_stats.num_regular_files;
  UMA_HISTOGRAM_COUNTS("GData.NumberOfRegularFiles",
                       uma_stats.num_regular_files);
  UMA_HISTOGRAM_COUNTS("GData.NumberOfHostedDocuments",
                       uma_stats.num_hosted_documents);
  UMA_HISTOGRAM_COUNTS("GData.NumberOfTotalFiles", num_total_files);
  const std::vector<int> all_entry_kinds = DocumentEntry::GetAllEntryKinds();
  for (FeedToFileResourceMapUmaStats::EntryKindToCountMap::const_iterator iter =
           uma_stats.num_files_with_entry_kind.begin();
       iter != uma_stats.num_files_with_entry_kind.end();
       ++iter) {
    const DocumentEntry::EntryKind kind = iter->first;
    const int count = iter->second;
    for (int i = 0; i < count; ++i) {
      UMA_HISTOGRAM_CUSTOM_ENUMERATION(
          "GData.EntryKind", kind, all_entry_kinds);
    }
  }
}

void GDataWapiFeedProcessor::ApplyFeedFromFileUrlMap(
    bool is_delta_feed,
    int64 feed_changestamp,
    FileResourceIdMap* file_map,
  std::set<FilePath>* changed_dirs) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(changed_dirs);

  if (!is_delta_feed) {  // Full update.
    directory_service_->root()->RemoveChildren();
    changed_dirs->insert(directory_service_->root()->GetFilePath());
  }
  directory_service_->set_largest_changestamp(feed_changestamp);

  scoped_ptr<GDataDirectoryService> orphaned_dir_service(
      new GDataDirectoryService);
  // Go through all entries generated by the feed and apply them to the local
  // snapshot of the file system.
  for (FileResourceIdMap::iterator it = file_map->begin();
       it != file_map->end();) {
    // Ensure that the entry is deleted, unless the ownership is explicitly
    // transferred by entry.release().
    scoped_ptr<GDataEntry> entry(it->second);
    DCHECK_EQ(it->first, entry->resource_id());
    // Erase the entry so the deleted entry won't be referenced.
    file_map->erase(it++);

    GDataEntry* old_entry =
        directory_service_->GetEntryByResourceId(entry->resource_id());
    GDataDirectory* dest_dir = NULL;
    if (entry->is_deleted()) {  // Deleted file/directory.
      DVLOG(1) << "Removing file " << entry->base_name();
      if (!old_entry)
        continue;

      dest_dir = old_entry->parent();
      if (!dest_dir) {
        NOTREACHED();
        continue;
      }
      RemoveEntryFromDirectoryAndCollectChangedDirectories(
          dest_dir, old_entry, changed_dirs);
    } else if (old_entry) {  // Change or move of existing entry.
      // Please note that entry rename is just a special case of change here
      // since name is just one of the properties that can change.
      DVLOG(1) << "Changed file " << entry->base_name();
      dest_dir = old_entry->parent();
      if (!dest_dir) {
        NOTREACHED();
        continue;
      }
      // Move children files over if we are dealing with directories.
      if (old_entry->AsGDataDirectory() && entry->AsGDataDirectory()) {
        entry->AsGDataDirectory()->TakeOverEntries(
            old_entry->AsGDataDirectory());
      }
      // Remove the old instance of this entry.
      RemoveEntryFromDirectoryAndCollectChangedDirectories(
          dest_dir, old_entry, changed_dirs);
      // Did we actually move the new file to another directory?
      if (dest_dir->resource_id() != entry->parent_resource_id()) {
        changed_dirs->insert(dest_dir->GetFilePath());
        dest_dir = FindDirectoryForNewEntry(entry.get(),
                                            *file_map,
                                            orphaned_dir_service.get());
      }
      DCHECK(dest_dir);
      AddEntryToDirectoryAndCollectChangedDirectories(
          entry.release(),
          dest_dir,
          orphaned_dir_service.get(),
          changed_dirs);
    } else {  // Adding a new file.
      dest_dir = FindDirectoryForNewEntry(entry.get(),
                                          *file_map,
                                          orphaned_dir_service.get());
      DCHECK(dest_dir);
      AddEntryToDirectoryAndCollectChangedDirectories(
          entry.release(),
          dest_dir,
          orphaned_dir_service.get(),
          changed_dirs);
    }

    // Record changed directory if this was a delta feed and the parent
    // directory is already properly rooted within its parent.
    if (dest_dir && (dest_dir->parent() ||
        dest_dir == directory_service_->root()) &&
        dest_dir != orphaned_dir_service->root() && is_delta_feed) {
      changed_dirs->insert(dest_dir->GetFilePath());
    }
  }
  // All entry must be erased from the map.
  DCHECK(file_map->empty());
}

// static
void GDataWapiFeedProcessor::AddEntryToDirectoryAndCollectChangedDirectories(
    GDataEntry* entry,
    GDataDirectory* directory,
    GDataDirectoryService* orphaned_dir_service,
    std::set<FilePath>* changed_dirs) {
  directory->AddEntry(entry);
  if (entry->AsGDataDirectory() && directory != orphaned_dir_service->root())
    changed_dirs->insert(entry->GetFilePath());
}

// static
void GDataWapiFeedProcessor::
RemoveEntryFromDirectoryAndCollectChangedDirectories(
    GDataDirectory* directory,
    GDataEntry* entry,
    std::set<FilePath>* changed_dirs) {
  // Get the list of all sub-directory paths, so we can notify their listeners
  // that they are smoked.
  GetChildDirectoryPaths(entry, changed_dirs);
  directory->RemoveEntry(entry);
}

GDataDirectory* GDataWapiFeedProcessor::FindDirectoryForNewEntry(
    GDataEntry* new_entry,
    const FileResourceIdMap& file_map,
    GDataDirectoryService* orphaned_dir_service) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  GDataDirectory* dir = NULL;
  // Added file.
  const std::string& parent_id = new_entry->parent_resource_id();
  if (parent_id.empty()) {
    dir = directory_service_->root();
    DVLOG(1) << "Root parent for " << new_entry->base_name();
  } else {
    GDataEntry* entry = directory_service_->GetEntryByResourceId(parent_id);
    dir = entry ? entry->AsGDataDirectory() : NULL;
    if (!dir) {
      // The parent directory was also added with this set of feeds.
      FileResourceIdMap::const_iterator find_iter =
          file_map.find(parent_id);
      dir = (find_iter != file_map.end() &&
             find_iter->second) ?
                find_iter->second->AsGDataDirectory() : NULL;
      if (dir) {
        DVLOG(1) << "Found parent for " << new_entry->base_name()
                 << " in file_map " << parent_id;
      } else {
        DVLOG(1) << "Adding orphan " << new_entry->GetFilePath().value();
        dir = orphaned_dir_service->root();
      }
    }
  }
  return dir;
}

GDataFileError GDataWapiFeedProcessor::FeedToFileResourceMap(
    const std::vector<DocumentFeed*>& feed_list,
    FileResourceIdMap* file_map,
    int64* feed_changestamp,
    FeedToFileResourceMapUmaStats* uma_stats) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(uma_stats);

  GDataFileError error = GDATA_FILE_OK;
  uma_stats->num_regular_files = 0;
  uma_stats->num_hosted_documents = 0;
  uma_stats->num_files_with_entry_kind.clear();
  for (size_t i = 0; i < feed_list.size(); ++i) {
    const DocumentFeed* feed = feed_list[i];

    // Get upload url from the root feed. Links for all other collections will
    // be handled in GDatadirectory::FromDocumentEntry();
    if (i == 0) {
      const Link* root_feed_upload_link =
          feed->GetLinkByType(Link::RESUMABLE_CREATE_MEDIA);
      if (root_feed_upload_link)
        directory_service_->root()->set_upload_url(
            root_feed_upload_link->href());
      *feed_changestamp = feed->largest_changestamp();
      DCHECK_GE(*feed_changestamp, 0);
    }

    for (ScopedVector<DocumentEntry>::const_iterator iter =
             feed->entries().begin();
         iter != feed->entries().end(); ++iter) {
      DocumentEntry* doc = *iter;
      GDataEntry* entry = directory_service_->FromDocumentEntry(*doc);
      // Some document entries don't map into files (i.e. sites).
      if (!entry)
        continue;
      // Count the number of files.
      GDataFile* as_file = entry->AsGDataFile();
      if (as_file) {
        if (as_file->is_hosted_document())
          ++uma_stats->num_hosted_documents;
        else
          ++uma_stats->num_regular_files;
        ++uma_stats->num_files_with_entry_kind[as_file->kind()];
      }

      FileResourceIdMap::iterator map_entry =
          file_map->find(entry->resource_id());

      // An entry with the same self link may already exist, so we need to
      // release the existing GDataEntry instance before overwriting the
      // entry with another GDataEntry instance.
      if (map_entry != file_map->end()) {
        LOG(WARNING) << "Found duplicate file "
                     << map_entry->second->base_name();

        delete map_entry->second;
        file_map->erase(map_entry);
      }
      file_map->insert(
          std::pair<std::string, GDataEntry*>(entry->resource_id(), entry));
    }
  }

  if (error != GDATA_FILE_OK) {
    // If the code above fails to parse a feed, any GDataEntry instance
    // added to |file_by_url| is not managed by a GDataDirectory instance,
    // so we need to explicitly release them here.
    STLDeleteValues(file_map);
  }

  return error;
}

}  // namespace gdata
