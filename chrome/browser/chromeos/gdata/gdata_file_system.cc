// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/gdata/gdata_file_system.h"

#include <errno.h>

#include <utility>

#include "base/bind.h"
#include "base/eintr_wrapper.h"
#include "base/file_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/message_loop.h"
#include "base/message_loop_proxy.h"
#include "base/platform_file.h"
#include "base/threading/platform_thread.h"
#include "base/synchronization/waitable_event.h"
#include "base/values.h"
#include "chrome/browser/chromeos/gdata/gdata_documents_service.h"
#include "chrome/browser/chromeos/gdata/gdata_download_observer.h"
#include "chrome/browser/chromeos/gdata/gdata_sync_client.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_paths_internal.h"
#include "content/public/browser/browser_thread.h"
#include "webkit/fileapi/file_system_file_util_proxy.h"
#include "webkit/fileapi/file_system_types.h"
#include "webkit/fileapi/file_system_util.h"

using content::BrowserThread;

namespace {

const FilePath::CharType kGDataRootDirectory[] = FILE_PATH_LITERAL("gdata");
const char kFeedField[] = "feed";
const char kWildCard[] = "*";
const char kLocallyModifiedFileExtension[] = "local";

const FilePath::CharType kGDataCacheVersionDir[] = FILE_PATH_LITERAL("v1");
const FilePath::CharType kGDataCacheMetaDir[] = FILE_PATH_LITERAL("meta");
const FilePath::CharType kGDataCachePinnedDir[] = FILE_PATH_LITERAL("pinned");
const FilePath::CharType kGDataCacheOutgoingDir[] =
    FILE_PATH_LITERAL("outgoing");
const FilePath::CharType kGDataCachePersistentDir[] =
    FILE_PATH_LITERAL("persistent");
const FilePath::CharType kGDataCacheTmpDir[] = FILE_PATH_LITERAL("tmp");
const FilePath::CharType kGDataCacheTmpDownloadsDir[] =
    FILE_PATH_LITERAL("tmp/downloads");
const FilePath::CharType kLastFeedFile[] = FILE_PATH_LITERAL("last_feed.json");
const char kGDataFileSystemToken[] = "GDataFileSystemToken";
const FilePath::CharType kAccountMetadataFile[] =
    FILE_PATH_LITERAL("account_metadata.json");
const FilePath::CharType kSymLinkToDevNull[] = FILE_PATH_LITERAL("/dev/null");

// Converts gdata error code into file platform error code.
base::PlatformFileError GDataToPlatformError(gdata::GDataErrorCode status) {
  switch (status) {
    case gdata::HTTP_SUCCESS:
    case gdata::HTTP_CREATED:
      return base::PLATFORM_FILE_OK;
    case gdata::HTTP_UNAUTHORIZED:
    case gdata::HTTP_FORBIDDEN:
      return base::PLATFORM_FILE_ERROR_ACCESS_DENIED;
    case gdata::HTTP_NOT_FOUND:
      return base::PLATFORM_FILE_ERROR_NOT_FOUND;
    case gdata::GDATA_PARSE_ERROR:
    case gdata::GDATA_FILE_ERROR:
      return base::PLATFORM_FILE_ERROR_ABORT;
    default:
      return base::PLATFORM_FILE_ERROR_FAILED;
  }
}

// Converts system error to file platform error code.
// This is copied and modified from base/platform_file_posix.cc.
// TODO(kuan): base/platform.h should probably export this.
base::PlatformFileError SystemToPlatformError(int error) {
  switch (error) {
    case 0:
      return base::PLATFORM_FILE_OK;
    case EACCES:
    case EISDIR:
    case EROFS:
    case EPERM:
      return base::PLATFORM_FILE_ERROR_ACCESS_DENIED;
    case ETXTBSY:
      return base::PLATFORM_FILE_ERROR_IN_USE;
    case EEXIST:
      return base::PLATFORM_FILE_ERROR_EXISTS;
    case ENOENT:
      return base::PLATFORM_FILE_ERROR_NOT_FOUND;
    case EMFILE:
      return base::PLATFORM_FILE_ERROR_TOO_MANY_OPENED;
    case ENOMEM:
      return base::PLATFORM_FILE_ERROR_NO_MEMORY;
    case ENOSPC:
      return base::PLATFORM_FILE_ERROR_NO_SPACE;
    case ENOTDIR:
      return base::PLATFORM_FILE_ERROR_NOT_A_DIRECTORY;
    case EINTR:
      return base::PLATFORM_FILE_ERROR_ABORT;
    default:
      return base::PLATFORM_FILE_ERROR_FAILED;
  }
}

//================================ Helper functions ============================

// Creates cache directory and its sub-directories if they don't exist.
// TODO(glotov): take care of this when the setup and cleanup part is landed,
// noting that these directories need to be created for development in linux box
// and unittest. (http://crosbug.com/27577)
base::PlatformFileError CreateCacheDirectories(
    const std::vector<FilePath>& paths_to_create) {
  base::PlatformFileError error = base::PLATFORM_FILE_OK;

  for (size_t i = 0; i < paths_to_create.size(); ++i) {
    if (file_util::DirectoryExists(paths_to_create[i]))
      continue;

    if (!file_util::CreateDirectory(paths_to_create[i])) {
      // Error creating this directory, record error and proceed with next one.
      error = SystemToPlatformError(errno);
      LOG(ERROR) << "Error creating dir " << paths_to_create[i].value()
                 << ": \"" << strerror(errno)
                 << "\", " << error;
    } else {
      DVLOG(1) << "Created dir " << paths_to_create[i].value();
    }
  }

  return error;
}

// Modifies cache state of file on IO thread pool, which involves:
// - moving file from |source_path| to |dest_path| if they're different
// - deleting symlink if |symlink_path| is not empty
// - creating symlink if |symlink_path| is not empty and |create_symlink| is
//   true.
base::PlatformFileError ModifyCacheState(
    const FilePath& source_path,
    const FilePath& dest_path,
    const FilePath& symlink_path,
    bool create_symlink) {
  // Move |source_path| to |dest_path| if they are different.
  if (source_path != dest_path) {
    if (!file_util::Move(source_path, dest_path)) {
      base::PlatformFileError error = SystemToPlatformError(errno);
      LOG(ERROR) << "Error moving " << source_path.value()
                 << " to " << dest_path.value()
                 << ": " << strerror(errno);
      return error;
    } else {
      DVLOG(1) << "Moved " << source_path.value()
               << " to " << dest_path.value();
    }
  } else {
    DVLOG(1) << "No need to move file: source = destination";
  }

  if (symlink_path.empty())
    return base::PLATFORM_FILE_OK;

  // Remove symlink regardless of |create_symlink| because creating a link will
  // not overwrite an existing one.

  // Cannot use file_util::Delete which uses stat64 to check if path exists
  // before deleting it.  If path is a symlink, stat64 dereferences it to the
  // target file, so it's in essence checking if the target file exists.
  // Here in this function, if |symlink_path| references |source_path| and
  // |source_path| has just been moved to |dest_path| (e.g. during unpinning),
  // symlink will dereference to a non-existent file.  This results in stat64
  // failing and file_util::Delete bailing out without deleting the symlink.
  // We clearly want the symlink deleted even if it dereferences to nothing.
  // Unfortunately, deleting the symlink before moving the files won't work for
  // the case where move operation fails, but the symlink has already been
  // deleted, which shouldn't happen.  An example scenario is where an existing
  // file is stored to cache and pinned for a specific resource id and md5, then
  // a non-existent file is stored to cache for the same resource id and md5.
  // The 2nd store-to-cache operation fails when moving files, but the symlink
  // created by previous pin operation has already been deleted.
  // We definitely want to keep the pinned state of the symlink if subsequent
  // operations fail.
  // This problem is filed at http://crbug.com/119430.

  // We try to save one file operation by not checking if link exists before
  // deleting it, so unlink may return error if link doesn't exist, but it
  // doesn't really matter to us.
  bool deleted = HANDLE_EINTR(unlink(symlink_path.value().c_str())) == 0;
  if (deleted) {
    DVLOG(1) << "Deleted symlink " << symlink_path.value();
  } else {
    // Since we didn't check if symlink exists before deleting it, don't log
    // if symlink doesn't exist.
    if (errno != ENOENT) {
      LOG(WARNING) << "Error deleting symlink " << symlink_path.value()
                   << ": " << strerror(errno);
    }
  }

  if (!create_symlink)
    return base::PLATFORM_FILE_OK;

  // Create new symlink to |dest_path|.
  if (!file_util::CreateSymbolicLink(dest_path, symlink_path)) {
    base::PlatformFileError error = SystemToPlatformError(errno);
    LOG(ERROR) << "Error creating symlink " << symlink_path.value()
               << " for " << dest_path.value()
               << ": " << strerror(errno);
    return error;
  } else {
    DVLOG(1) << "Created symlink " << symlink_path.value()
             << " to " << dest_path.value();
  }

  return base::PLATFORM_FILE_OK;
}

// Deletes all files that match |path_to_delete_pattern| except for
// |path_to_keep| on IO thread pool.
// If |path_to_keep| is empty, all files in |path_to_delete_pattern| are
// deleted.
void DeleteFilesSelectively(const FilePath& path_to_delete_pattern,
                            const FilePath& path_to_keep) {
  // Enumerate all files in directory of |path_to_delete_pattern| that match
  // base name of |path_to_delete_pattern|.
  // If a file is not |path_to_keep|, delete it.
  bool success = true;
  file_util::FileEnumerator enumerator(
      path_to_delete_pattern.DirName(),
      false,  // not recursive
      static_cast<file_util::FileEnumerator::FileType>(
          file_util::FileEnumerator::FILES |
          file_util::FileEnumerator::SHOW_SYM_LINKS),
      path_to_delete_pattern.BaseName().value());
  for (FilePath current = enumerator.Next(); !current.empty();
       current = enumerator.Next()) {
    // If |path_to_keep| is not empty and same as current, don't delete it.
    if (!path_to_keep.empty() && current == path_to_keep)
      continue;

    success = HANDLE_EINTR(unlink(current.value().c_str())) == 0;
    if (!success)
      DVLOG(1) << "Error deleting " << current.value();
    else
      DVLOG(1) << "Deleted " << current.value();
  }
}

}  // namespace

namespace gdata {

// FindFileDelegate class implementation.

FindFileDelegate::~FindFileDelegate() {
}

// FindFileCallbackRelayDelegate class implementation.
// This class is used to relay calls between sync and async versions
// of FindFileByPath(Sync|Async) calls.
class FindFileCallbackRelayDelegate : public FindFileDelegate {
 public:
  explicit FindFileCallbackRelayDelegate(const FindFileCallback& callback);
  virtual ~FindFileCallbackRelayDelegate();

 private:
  // FindFileDelegate overrides.
  virtual void OnDone(base::PlatformFileError error,
                      const FilePath& directory_path,
                      GDataFileBase* file) OVERRIDE;

  const FindFileCallback callback_;
};

FindFileCallbackRelayDelegate::FindFileCallbackRelayDelegate(
    const FindFileCallback& callback) : callback_(callback) {
}

FindFileCallbackRelayDelegate::~FindFileCallbackRelayDelegate() {
}

void FindFileCallbackRelayDelegate::OnDone(base::PlatformFileError error,
                                           const FilePath& directory_path,
                                           GDataFileBase* file) {
  if (!callback_.is_null()) {
    callback_.Run(error, directory_path, file);
  }
}

// ReadOnlyFindFileDelegate class implementation.

ReadOnlyFindFileDelegate::ReadOnlyFindFileDelegate() : file_(NULL) {
}

void ReadOnlyFindFileDelegate::OnDone(base::PlatformFileError error,
                                      const FilePath& directory_path,
                                      GDataFileBase* file) {
  DCHECK(!file_);
  if (error == base::PLATFORM_FILE_OK)
    file_ = file;
  else
    file_ = NULL;
}

// GDataFileProperties struct implementation.

GDataFileProperties::GDataFileProperties() : is_hosted_document(false) {
}

GDataFileProperties::~GDataFileProperties() {
}

// GDataFileSystem::CreateDirectoryParams struct implementation.

GDataFileSystem::CreateDirectoryParams::CreateDirectoryParams(
    const FilePath& created_directory_path,
    const FilePath& target_directory_path,
    bool is_exclusive,
    bool is_recursive,
    const FileOperationCallback& callback)
    : created_directory_path(created_directory_path),
      target_directory_path(target_directory_path),
      is_exclusive(is_exclusive),
      is_recursive(is_recursive),
      callback(callback) {
}


GDataFileSystem::CreateDirectoryParams::~CreateDirectoryParams() {
}

// GDataFileSystem::ModifyCacheStateParams struct implementation.

GDataFileSystem::ModifyCacheStateParams::ModifyCacheStateParams(
    const std::string& in_resource_id,
    const std::string& in_md5,
    const FilePath& in_source_path,
    const CacheOperationCallback& in_final_callback,
    const CacheOperationIntermediateCallback& in_intermediate_callback,
    scoped_refptr<base::MessageLoopProxy> in_relay_proxy)
    : resource_id(in_resource_id),
      md5(in_md5),
      source_path(in_source_path),
      final_callback(in_final_callback),
      intermediate_callback(in_intermediate_callback),
      relay_proxy(in_relay_proxy) {
}

GDataFileSystem::ModifyCacheStateParams::~ModifyCacheStateParams() {
}

//=================== GetFileFromCacheParams implementation ===================

GDataFileSystem::GetFileFromCacheParams::GetFileFromCacheParams(
    const FilePath& virtual_file_path,
    const FilePath& local_tmp_path,
    const GURL& content_url,
    const std::string& resource_id,
    const std::string& md5,
    scoped_refptr<base::MessageLoopProxy> proxy,
    const GetFileCallback& callback)
    : virtual_file_path(virtual_file_path),
      local_tmp_path(local_tmp_path),
      content_url(content_url),
      resource_id(resource_id),
      md5(md5),
      proxy(proxy),
      callback(callback) {
}

GDataFileSystem::GetFileFromCacheParams::~GetFileFromCacheParams() {
}

// GDataFileSystem class implementatsion.

GDataFileSystem::GDataFileSystem(Profile* profile,
                                 DocumentsServiceInterface* documents_service)
    : profile_(profile),
      documents_service_(documents_service),
      on_io_completed_(new base::WaitableEvent(
          true /* manual reset */, false /* initially not signaled */)),
      cache_initialization_started_(false),
      in_shutdown_(false),
      ui_weak_ptr_factory_(ALLOW_THIS_IN_INITIALIZER_LIST(
          new base::WeakPtrFactory<GDataFileSystem>(this))),
      ui_weak_ptr_(ui_weak_ptr_factory_->GetWeakPtr()) {
  // Should be created from the file browser extension API on UI thread.
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
}

void GDataFileSystem::Initialize() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  FilePath cache_base_path;
  chrome::GetUserCacheDirectory(profile_->GetPath(), &cache_base_path);
  gdata_cache_path_ = cache_base_path.Append(chrome::kGDataCacheDirname);
  gdata_cache_path_ = gdata_cache_path_.Append(kGDataCacheVersionDir);
  // Insert into |cache_paths_| in order defined in enum CacheSubDirectoryType.
  cache_paths_.push_back(gdata_cache_path_.Append(kGDataCacheMetaDir));
  cache_paths_.push_back(gdata_cache_path_.Append(kGDataCachePinnedDir));
  cache_paths_.push_back(gdata_cache_path_.Append(kGDataCacheOutgoingDir));
  cache_paths_.push_back(gdata_cache_path_.Append(kGDataCachePersistentDir));
  cache_paths_.push_back(gdata_cache_path_.Append(kGDataCacheTmpDir));
  cache_paths_.push_back(gdata_cache_path_.Append(kGDataCacheTmpDownloadsDir));

  documents_service_->Initialize(profile_);

  root_.reset(new GDataRootDirectory(this));
  root_->set_file_name(kGDataRootDirectory);
}

GDataFileSystem::~GDataFileSystem() {
  // Should be deleted on IO thread by GDataSystemService.
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  // io_weak_ptr_factory_ must be deleted on IO thread.
  io_weak_ptr_factory_.reset();
  // documents_service_ must be deleted on IO thread, as it also owns
  // WeakPtrFactory bound to IO thread.
  documents_service_.reset();
}

void GDataFileSystem::ShutdownOnUIThread() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  // Cancel all the in-flight operations.
  // This asynchronously cancels the URL fetch operations.
  documents_service_->CancelAll();
  documents_service_.reset();

  // ui_weak_ptr_factory_ must be deleted on UI thread.
  ui_weak_ptr_factory_.reset();

  // In case an IO task is in progress, wait for its completion before
  // destructing because it accesses data members.
  bool need_to_wait = false;
  {  // Lock to update in_shutdown_ and access cache_initialization_started_,
     // but need to release before waiting for |on_io_completed_| signal so that
     // IO tasks won't deadlock waiting to lock to update data members and
     // signal on_io_completed_.
    base::AutoLock lock(lock_);

    // |in_shutdown_| flag is used to cancel in-flight operations on the
    // sequenced IO thread pool.  We'll need to wait for the first operation to
    // complete (there is no way to cancel this one if it's already running),
    // but subsequent operations are canceled if this flag is set to true.
    in_shutdown_ = true;

    // Cache initialization is the very first IO task that's posted to the
    // sequenced IO thread pool, so as long as |cache_initialization_started_|
    // is set, there could be one IO task running and we need to wait for its
    // completion.
    need_to_wait = cache_initialization_started_;
  }

  if (need_to_wait)
    on_io_completed_->Wait();

  // Lock to let root destroy cache map and resource map.
  base::AutoLock lock(lock_);
  root_.reset(NULL);
}

void GDataFileSystem::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void GDataFileSystem::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void GDataFileSystem::Authenticate(const AuthStatusCallback& callback) {
  // TokenFetcher, used in DocumentsService, must be run on UI thread.
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  documents_service_->Authenticate(callback);
}

void GDataFileSystem::FindFileByPathSync(
    const FilePath& search_file_path,
    FindFileDelegate* delegate) {
  base::AutoLock lock(lock_);
  UnsafeFindFileByPath(search_file_path, delegate);
}

void GDataFileSystem::FindFileByPathAsync(
    const FilePath& search_file_path,
    const FindFileCallback& callback) {
  scoped_refptr<base::MessageLoopProxy> proxy(
      base::MessageLoopProxy::current());
  base::AutoLock lock(lock_);
  if (root_->origin() == UNINITIALIZED) {
    // Load root feed from this disk cache. Upon completion, kick off server
    // fetching.
    LoadRootFeedFromCache(search_file_path,
                          true,     // load_from_server
                          proxy,
                          callback);
    return;
  } else if (root_->NeedsRefresh()) {
    // If content is stale or from disk from cache, fetch content from
    // the server.
    LoadFeedFromServer(search_file_path, proxy, callback);
    return;
  }
  proxy->PostTask(FROM_HERE,
                  base::Bind(&GDataFileSystem::FindFileByPathOnCallingThread,
                             GetWeakPtrForCurrentThread(),
                             search_file_path,
                             callback));
}

void GDataFileSystem::FindFileByPathOnCallingThread(
    const FilePath& search_file_path,
    const FindFileCallback& callback) {
  FindFileCallbackRelayDelegate delegate(callback);
  FindFileByPathSync(search_file_path, &delegate);
}

void GDataFileSystem::LoadFeedFromServer(
    const FilePath& search_file_path,
    scoped_refptr<base::MessageLoopProxy> proxy,
    const FindFileCallback& callback) {
  // ...then also kick off document feed fetching from the server as well.
  // |feed_list| will contain the list of all collected feed updates that
  // we will receive through calls of DocumentsService::GetDocuments().
  scoped_ptr<ListValue> feed_list(new ListValue());
  // Kick off document feed fetching here if we don't have complete data
  // to finish this call.
  documents_service_->GetDocuments(
      GURL(),   // root feed start.
      base::Bind(&GDataFileSystem::OnGetDocuments,
                 GetWeakPtrForCurrentThread(),
                 search_file_path,
                 base::Passed(&feed_list),
                 proxy,
                 callback));
}

void GDataFileSystem::TransferFile(const FilePath& local_file_path,
                                   const FilePath& remote_dest_file_path,
                                   const FileOperationCallback& callback) {
  // TODO(zelidrag): Wire this with GDataUploader stack.
  if (!callback.is_null()) {
    MessageLoop::current()->PostTask(FROM_HERE,
        base::Bind(callback, base::PLATFORM_FILE_ERROR_NOT_EMPTY));
  }
  return;
}

void GDataFileSystem::Copy(const FilePath& src_file_path,
                           const FilePath& dest_file_path,
                           const FileOperationCallback& callback) {
  base::PlatformFileError error = base::PLATFORM_FILE_OK;
  FilePath dest_parent_path = dest_file_path.DirName();

  base::AutoLock lock(lock_);
  GDataFileBase* src_file = GetGDataFileInfoFromPath(src_file_path);
  GDataFileBase* dest_parent = GetGDataFileInfoFromPath(dest_parent_path);
  if (!src_file || !dest_parent) {
    error = base::PLATFORM_FILE_ERROR_NOT_FOUND;
  } else {
    // TODO(benchan): Implement copy for regular files and directories.
    // To copy a regular file, we need to first download the file and
    // then upload it, which is not yet implemented. Also, in the interim,
    // we handle recursive directory copy in the file manager.
    if (!src_file->AsGDataFile() ||
        !src_file->AsGDataFile()->is_hosted_document()) {
      error = base::PLATFORM_FILE_ERROR_INVALID_OPERATION;
    } else if (!dest_parent->AsGDataDirectory()) {
      error = base::PLATFORM_FILE_ERROR_NOT_A_DIRECTORY;
    }
  }

  if (error != base::PLATFORM_FILE_OK) {
    if (!callback.is_null())
      MessageLoop::current()->PostTask(FROM_HERE, base::Bind(callback, error));

    return;
  }

  FilePathUpdateCallback add_file_to_directory_callback =
      base::Bind(&GDataFileSystem::AddFileToDirectory,
                 GetWeakPtrForCurrentThread(),
                 dest_parent_path,
                 callback);

  documents_service_->CopyDocument(
      src_file->self_url(),
      // Drop the document extension, which should not be in the document title.
      dest_file_path.BaseName().RemoveExtension().value(),
      base::Bind(&GDataFileSystem::OnCopyDocumentCompleted,
                 GetWeakPtrForCurrentThread(),
                 add_file_to_directory_callback));
}

void GDataFileSystem::Rename(const FilePath& file_path,
                             const FilePath::StringType& new_name,
                             const FilePathUpdateCallback& callback) {
  // It is a no-op if the file is renamed to the same name.
  if (file_path.BaseName().value() == new_name) {
    if (!callback.is_null()) {
      MessageLoop::current()->PostTask(
          FROM_HERE, base::Bind(callback, base::PLATFORM_FILE_OK, file_path));
    }
    return;
  }

  base::AutoLock lock(lock_);
  GDataFileBase* file = GetGDataFileInfoFromPath(file_path);
  if (!file) {
    if (!callback.is_null()) {
      MessageLoop::current()->PostTask(FROM_HERE,
          base::Bind(callback, base::PLATFORM_FILE_ERROR_NOT_FOUND, file_path));
    }
    return;
  }

  // Drop the .g<something> extension from |new_name| if the file being
  // renamed is a hosted document and |new_name| has the same .g<something>
  // extension as the file.
  FilePath::StringType file_name = new_name;
  if (file->AsGDataFile() && file->AsGDataFile()->is_hosted_document()) {
    FilePath new_file(file_name);
    if (new_file.Extension() == file->AsGDataFile()->document_extension()) {
      file_name = new_file.RemoveExtension().value();
    }
  }

  documents_service_->RenameResource(
      file->self_url(),
      file_name,
      base::Bind(&GDataFileSystem::OnRenameResourceCompleted,
                 GetWeakPtrForCurrentThread(),
                 file_path,
                 file_name,
                 callback));
}

void GDataFileSystem::Move(const FilePath& src_file_path,
                           const FilePath& dest_file_path,
                           const FileOperationCallback& callback) {
  base::PlatformFileError error = base::PLATFORM_FILE_OK;
  FilePath dest_parent_path = dest_file_path.DirName();

  {
    // This scoped lock needs to be released before calling Rename() below.
    base::AutoLock lock(lock_);
    GDataFileBase* src_file = GetGDataFileInfoFromPath(src_file_path);
    GDataFileBase* dest_parent = GetGDataFileInfoFromPath(dest_parent_path);
    if (!src_file || !dest_parent) {
      error = base::PLATFORM_FILE_ERROR_NOT_FOUND;
    } else {
      if (!dest_parent->AsGDataDirectory())
        error = base::PLATFORM_FILE_ERROR_NOT_A_DIRECTORY;
    }

    if (error != base::PLATFORM_FILE_OK) {
      if (!callback.is_null()) {
        MessageLoop::current()->PostTask(FROM_HERE,
            base::Bind(callback, error));
      }
      return;
    }
  }

  // If the file/directory is moved to the same directory, just rename it.
  if (src_file_path.DirName() == dest_parent_path) {
    FilePathUpdateCallback final_file_path_update_callback =
        base::Bind(&GDataFileSystem::OnFilePathUpdated,
                   GetWeakPtrForCurrentThread(),
                   callback);

    Rename(src_file_path, dest_file_path.BaseName().value(),
           final_file_path_update_callback);
    return;
  }

  // Otherwise, the move operation involves three steps:
  // 1. Renames the file at |src_file_path| to basename(|dest_file_path|)
  //    within the same directory. The rename operation is a no-op if
  //    basename(|src_file_path|) equals to basename(|dest_file_path|).
  // 2. Removes the file from its parent directory (the file is not deleted),
  //    which effectively moves the file to the root directory.
  // 3. Adds the file to the parent directory of |dest_file_path|, which
  //    effectively moves the file from the root directory to the parent
  //    directory of |dest_file_path|.
  FilePathUpdateCallback add_file_to_directory_callback =
      base::Bind(&GDataFileSystem::AddFileToDirectory,
                 GetWeakPtrForCurrentThread(),
                 dest_file_path.DirName(),
                 callback);

  FilePathUpdateCallback remove_file_from_directory_callback =
      base::Bind(&GDataFileSystem::RemoveFileFromDirectory,
                 GetWeakPtrForCurrentThread(),
                 src_file_path.DirName(),
                 add_file_to_directory_callback);

  Rename(src_file_path, dest_file_path.BaseName().value(),
         remove_file_from_directory_callback);
}

void GDataFileSystem::AddFileToDirectory(const FilePath& dir_path,
                                         const FileOperationCallback& callback,
                                         base::PlatformFileError error,
                                         const FilePath& file_path) {
  base::AutoLock lock(lock_);
  GDataFileBase* file = GetGDataFileInfoFromPath(file_path);
  GDataFileBase* dir = GetGDataFileInfoFromPath(dir_path);
  if (error == base::PLATFORM_FILE_OK) {
    if (!file || !dir) {
      error = base::PLATFORM_FILE_ERROR_NOT_FOUND;
    } else {
      if (!dir->AsGDataDirectory())
        error = base::PLATFORM_FILE_ERROR_NOT_A_DIRECTORY;
    }
  }

  // Returns if there is an error or |dir_path| is the root directory.
  if (error != base::PLATFORM_FILE_OK || dir->AsGDataRootDirectory()) {
    if (!callback.is_null())
      MessageLoop::current()->PostTask(FROM_HERE, base::Bind(callback, error));

    return;
  }

  documents_service_->AddResourceToDirectory(
      dir->content_url(),
      file->self_url(),
      base::Bind(&GDataFileSystem::OnAddFileToDirectoryCompleted,
                 GetWeakPtrForCurrentThread(),
                 callback,
                 file_path,
                 dir_path));
}

void GDataFileSystem::RemoveFileFromDirectory(
    const FilePath& dir_path,
    const FilePathUpdateCallback& callback,
    base::PlatformFileError error,
    const FilePath& file_path) {
  base::AutoLock lock(lock_);
  GDataFileBase* file = GetGDataFileInfoFromPath(file_path);
  GDataFileBase* dir = GetGDataFileInfoFromPath(dir_path);
  if (error == base::PLATFORM_FILE_OK) {
    if (!file || !dir) {
      error = base::PLATFORM_FILE_ERROR_NOT_FOUND;
    } else {
      if (!dir->AsGDataDirectory())
        error = base::PLATFORM_FILE_ERROR_NOT_A_DIRECTORY;
    }
  }

  // Returns if there is an error or |dir_path| is the root directory.
  if (error != base::PLATFORM_FILE_OK || dir->AsGDataRootDirectory()) {
    if (!callback.is_null()) {
      MessageLoop::current()->PostTask(FROM_HERE,
          base::Bind(callback, error, file_path));
    }
    return;
  }

  documents_service_->RemoveResourceFromDirectory(
      dir->content_url(),
      file->self_url(),
      file->resource_id(),
      base::Bind(&GDataFileSystem::OnRemoveFileFromDirectoryCompleted,
                 GetWeakPtrForCurrentThread(),
                 callback,
                 file_path,
                 dir_path));
}

void GDataFileSystem::Remove(const FilePath& file_path,
    bool is_recursive,
    const FileOperationCallback& callback) {
  base::AutoLock lock(lock_);
  GDataFileBase* file_info = GetGDataFileInfoFromPath(file_path);
  if (!file_info) {
    if (!callback.is_null()) {
      MessageLoop::current()->PostTask(
          FROM_HERE,
          base::Bind(callback, base::PLATFORM_FILE_ERROR_NOT_FOUND));
    }
    return;
  }

  documents_service_->DeleteDocument(
      file_info->self_url(),
      base::Bind(&GDataFileSystem::OnRemovedDocument,
                 GetWeakPtrForCurrentThread(),
                 callback,
                 file_path));
}

void GDataFileSystem::CreateDirectory(
    const FilePath& directory_path,
    bool is_exclusive,
    bool is_recursive,
    const FileOperationCallback& callback) {
  FilePath last_parent_dir_path;
  FilePath first_missing_path;
  GURL last_parent_dir_url;
  FindMissingDirectoryResult result =
      FindFirstMissingParentDirectory(directory_path,
                                      &last_parent_dir_url,
                                      &first_missing_path);
  switch (result) {
    case FOUND_INVALID: {
      if (!callback.is_null()) {
        MessageLoop::current()->PostTask(FROM_HERE,
            base::Bind(callback, base::PLATFORM_FILE_ERROR_NOT_FOUND));
      }

      return;
    }
    case DIRECTORY_ALREADY_PRESENT: {
      if (!callback.is_null()) {
        MessageLoop::current()->PostTask(FROM_HERE,
            base::Bind(callback,
                       is_exclusive ? base::PLATFORM_FILE_ERROR_EXISTS :
                                      base::PLATFORM_FILE_OK));
      }

      return;
    }
    case FOUND_MISSING: {
      // There is a missing folder to be created here, move on with the rest of
      // this function.
      break;
    }
    default: {
      NOTREACHED();
      break;
    }
  }

  // Do we have a parent directory here as well? We can't then create target
  // directory if this is not a recursive operation.
  if (directory_path !=  first_missing_path && !is_recursive) {
    if (!callback.is_null()) {
      MessageLoop::current()->PostTask(FROM_HERE,
           base::Bind(callback, base::PLATFORM_FILE_ERROR_NOT_FOUND));
    }
    return;
  }

  documents_service_->CreateDirectory(
      last_parent_dir_url,
      first_missing_path.BaseName().value(),
      base::Bind(&GDataFileSystem::OnCreateDirectoryCompleted,
                 GetWeakPtrForCurrentThread(),
                 CreateDirectoryParams(
                     first_missing_path,
                     directory_path,
                     is_exclusive,
                     is_recursive,
                     callback)));
}

// static
void GDataFileSystem::CreateDocumentJsonFileOnIOThreadPool(
    const GURL& edit_url,
    const std::string& resource_id,
    const GetFileCallback& callback,
    scoped_refptr<base::MessageLoopProxy> relay_proxy) {
  base::PlatformFileError error = base::PLATFORM_FILE_ERROR_FAILED;
  FilePath temp_file;

  if (file_util::CreateTemporaryFile(&temp_file)) {
    std::string document_content = base::StringPrintf(
        "{\"url\": \"%s\", \"resource_id\": \"%s\"}",
        edit_url.spec().c_str(), resource_id.c_str());
    int document_size = static_cast<int>(document_content.size());
    if (file_util::WriteFile(temp_file, document_content.data(),
                             document_size) == document_size) {
      error = base::PLATFORM_FILE_OK;
    }
  }

  if (!callback.is_null()) {
    if (error != base::PLATFORM_FILE_OK)
      temp_file.clear();

    relay_proxy->PostTask(FROM_HERE,
        base::Bind(callback, error, temp_file, HOSTED_DOCUMENT));
  }
}

void GDataFileSystem::GetFile(const FilePath& file_path,
                              const GetFileCallback& callback) {
  GDataFileProperties file_properties;
  if (!GetFileInfoFromPath(file_path, &file_properties)) {
    if (!callback.is_null()) {
      MessageLoop::current()->PostTask(
          FROM_HERE,
          base::Bind(callback,
                     base::PLATFORM_FILE_ERROR_NOT_FOUND,
                     FilePath(),
                     REGULAR_FILE));
    }
    return;
  }

  // For a hosted document, we create a special JSON file to represent the
  // document instead of fetching the document content in one of the exported
  // formats. The JSON file contains the edit URL and resource ID of the
  // document.
  if (file_properties.is_hosted_document) {
    BrowserThread::PostBlockingPoolTask(FROM_HERE,
        base::Bind(&GDataFileSystem::CreateDocumentJsonFileOnIOThreadPool,
                   file_properties.edit_url,
                   file_properties.resource_id,
                   callback,
                   base::MessageLoopProxy::current()));
    return;
  }

  // Returns absolute path of the file if it were cached or to be cached.
  FilePath local_tmp_path = GetCacheFilePath(file_properties.resource_id,
                                             file_properties.file_md5,
                                             GDataRootDirectory::CACHE_TYPE_TMP,
                                             CACHED_FILE_FROM_SERVER);
  GetFromCache(file_properties.resource_id, file_properties.file_md5,
               base::Bind(
                   &GDataFileSystem::OnGetFileFromCache,
                   GetWeakPtrForCurrentThread(),
                   GetFileFromCacheParams(file_path,
                                          local_tmp_path,
                                          file_properties.content_url,
                                          file_properties.resource_id,
                                          file_properties.file_md5,
                                          base::MessageLoopProxy::current(),
                                          callback)));
}

void GDataFileSystem::OnGetFileFromCache(const GetFileFromCacheParams& params,
                                         base::PlatformFileError error,
                                         const std::string& resource_id,
                                         const std::string& md5,
                                         const FilePath& gdata_file_path,
                                         const FilePath& cache_file_path) {
  // Have we found the file in cache? If so, return it back to the caller.
  if (error == base::PLATFORM_FILE_OK) {
    if (!params.callback.is_null()) {
      params.proxy->PostTask(FROM_HERE,
                             base::Bind(params.callback,
                                        error,
                                        cache_file_path,
                                        REGULAR_FILE));
    }

    return;
  }

  // If cache file is not found, try to download it from the server instead.
  documents_service_->DownloadFile(
      params.virtual_file_path,
      params.local_tmp_path,
      params.content_url,
      base::Bind(&GDataFileSystem::OnFileDownloaded,
                 GetWeakPtrForCurrentThread(),
                 params));
}

void GDataFileSystem::InitiateUpload(
    const std::string& file_name,
    const std::string& content_type,
    int64 content_length,
    const FilePath& destination_directory,
    const FilePath& virtual_path,
    const InitiateUploadCallback& callback) {
  GURL destination_directory_url =
      GetUploadUrlForDirectory(destination_directory);

  if (destination_directory_url.is_empty()) {
    if (!callback.is_null()) {
      MessageLoop::current()->PostTask(
          FROM_HERE,
          base::Bind(callback,
                     HTTP_BAD_REQUEST, GURL()));
    }
    return;
  }

  documents_service_->InitiateUpload(
          InitiateUploadParams(file_name,
                                 content_type,
                                 content_length,
                                 destination_directory_url,
                                 virtual_path),
          base::Bind(&GDataFileSystem::OnUploadLocationReceived,
                     GetWeakPtrForCurrentThread(),
                     callback,
                     // MessageLoopProxy is used to run |callback| on the
                     // thread where this function was called.
                     base::MessageLoopProxy::current()));
}

void GDataFileSystem::OnUploadLocationReceived(
    const InitiateUploadCallback& callback,
    scoped_refptr<base::MessageLoopProxy> message_loop_proxy,
    GDataErrorCode code,
    const GURL& upload_location) {

  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  if (!callback.is_null()) {
    message_loop_proxy->PostTask(FROM_HERE, base::Bind(callback, code,
                                                       upload_location));
  }
}

void GDataFileSystem::ResumeUpload(
    const ResumeUploadParams& params,
    const ResumeFileUploadCallback& callback) {
  documents_service_->ResumeUpload(
          params,
          base::Bind(&GDataFileSystem::OnResumeUpload,
                     GetWeakPtrForCurrentThread(),
                     base::MessageLoopProxy::current(),
                     callback));
}

void GDataFileSystem::OnResumeUpload(
    scoped_refptr<base::MessageLoopProxy> message_loop_proxy,
    const ResumeFileUploadCallback& callback,
    const ResumeUploadResponse& response,
    scoped_ptr<DocumentEntry> new_entry) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  if (!callback.is_null())
    message_loop_proxy->PostTask(FROM_HERE,
        base::Bind(callback, response, base::Passed(&new_entry)));
}


void GDataFileSystem::UnsafeFindFileByPath(
    const FilePath& file_path,
    FindFileDelegate* delegate) {
  DCHECK(delegate);
  lock_.AssertAcquired();

  std::vector<FilePath::StringType> components;
  file_path.GetComponents(&components);

  GDataDirectory* current_dir = root_.get();
  FilePath directory_path;
  for (size_t i = 0; i < components.size() && current_dir; i++) {
    directory_path = directory_path.Append(current_dir->file_name());

    // Last element must match, if not last then it must be a directory.
    if (i == components.size() - 1) {
      if (current_dir->file_name() == components[i])
        delegate->OnDone(base::PLATFORM_FILE_OK, directory_path, current_dir);
      else
        delegate->OnDone(base::PLATFORM_FILE_ERROR_NOT_FOUND, FilePath(), NULL);

      return;
    }

    // Not the last part of the path, search for the next segment.
    GDataFileCollection::const_iterator file_iter =
        current_dir->children().find(components[i + 1]);
    if (file_iter == current_dir->children().end()) {
      delegate->OnDone(base::PLATFORM_FILE_ERROR_NOT_FOUND, FilePath(), NULL);
      return;
    }

    // Found file, must be the last segment.
    if (file_iter->second->file_info().is_directory) {
      // Found directory, continue traversal.
      current_dir = file_iter->second->AsGDataDirectory();
    } else {
      if ((i + 1) == (components.size() - 1)) {
        delegate->OnDone(base::PLATFORM_FILE_OK,
                         directory_path,
                         file_iter->second);
      } else {
        delegate->OnDone(base::PLATFORM_FILE_ERROR_NOT_FOUND, FilePath(), NULL);
      }

      return;
    }
  }
  delegate->OnDone(base::PLATFORM_FILE_ERROR_NOT_FOUND, FilePath(), NULL);
}

bool GDataFileSystem::GetFileInfoFromPath(
    const FilePath& file_path, GDataFileProperties* properties) {
  DCHECK(properties);
  base::AutoLock lock(lock_);
  GDataFileBase* file = GetGDataFileInfoFromPath(file_path);
  if (!file)
    return false;

  properties->file_info = file->file_info();
  properties->resource_id = file->resource_id();

  GDataFile* regular_file = file->AsGDataFile();
  if (regular_file) {
    properties->file_md5 = regular_file->file_md5();
    properties->content_url = regular_file->content_url();
    properties->edit_url = regular_file->edit_url();
    properties->is_hosted_document = regular_file->is_hosted_document();
  }
  return true;
}

FilePath GDataFileSystem::GetGDataCacheTmpDirectory() const {
  return cache_paths_[GDataRootDirectory::CACHE_TYPE_TMP];
}

FilePath GDataFileSystem::GetGDataTempDownloadFolderPath() const {
  return cache_paths_[GDataRootDirectory::CACHE_TYPE_TMP_DOWNLOADS];
}

FilePath GDataFileSystem::GetGDataCachePinnedDirectory() const {
  return cache_paths_[GDataRootDirectory::CACHE_TYPE_PINNED];
}

FilePath GDataFileSystem::GetGDataCachePersistentDirectory() const {
  return cache_paths_[GDataRootDirectory::CACHE_TYPE_PERSISTENT];
}

base::WeakPtr<GDataFileSystem> GDataFileSystem::GetWeakPtrForCurrentThread() {
  if (BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    return ui_weak_ptr_factory_->GetWeakPtr();
  } else if (BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    if (!io_weak_ptr_factory_.get()) {
      io_weak_ptr_factory_.reset(
          new base::WeakPtrFactory<GDataFileSystem>(this));
    }
    return io_weak_ptr_factory_->GetWeakPtr();
  }

  NOTREACHED() << "Called on an unexpected thread: "
               << base::PlatformThread::CurrentId();
  return ui_weak_ptr_factory_->GetWeakPtr();
}

GDataFileBase* GDataFileSystem::GetGDataFileInfoFromPath(
    const FilePath& file_path) {
  lock_.AssertAcquired();
  // Find directory element within the cached file system snapshot.
  ReadOnlyFindFileDelegate find_delegate;
  UnsafeFindFileByPath(file_path, &find_delegate);
  return find_delegate.file();
}

void GDataFileSystem::GetFromCacheForPath(
    const FilePath& gdata_file_path,
    const GetFromCacheCallback& callback) {
  std::string resource_id;
  std::string md5;

  {  // Lock to use GetGDataFileInfoFromPath and returned pointer, but need to
     // release before GetFromCache.
    base::AutoLock lock(lock_);
    GDataFileBase* file_base = GetGDataFileInfoFromPath(gdata_file_path);

    if (file_base && file_base->AsGDataFile()) {
      GDataFile* file = file_base->AsGDataFile();
      resource_id = file->resource_id();
      md5 = file->file_md5();
    } else {
      // Invoke |callback| with error.
      if (!callback.is_null()) {
          base::MessageLoopProxy::current()->PostTask(
              FROM_HERE,
              base::Bind(callback,
                         base::PLATFORM_FILE_ERROR_NOT_FOUND,
                         std::string(),
                         std::string(),
                         gdata_file_path,
                         FilePath()));
      }
      return;
    }
  }

  GetFromCacheInternal(resource_id, md5, gdata_file_path, callback);
}

void GDataFileSystem::GetCacheState(const std::string& resource_id,
                                    const std::string& md5,
                                    const GetCacheStateCallback& callback) {
  // This method originates from GDataFile::GetCacheState, which already locks,
  // so we shouldn't lock here.
  UnsafeInitializeCacheIfNecessary();

  PostBlockingPoolSequencedTask(
      kGDataFileSystemToken,
      FROM_HERE,
      base::Bind(&GDataFileSystem::GetCacheStateOnIOThreadPool,
                 base::Unretained(this),
                 resource_id,
                 md5,
                 callback,
                 base::Bind(&GDataFileSystem::OnGetCacheState,
                            GetWeakPtrForCurrentThread()),
                 base::MessageLoopProxy::current()));
}

void GDataFileSystem::GetAvailableSpace(
    const GetAvailableSpaceCallback& callback) {
  documents_service_->GetAccountMetadata(
      base::Bind(&GDataFileSystem::OnGetAvailableSpace,
                 GetWeakPtrForCurrentThread(),
                 callback));
}

void GDataFileSystem::OnGetAvailableSpace(
    const GetAvailableSpaceCallback& callback,
    GDataErrorCode status,
    scoped_ptr<base::Value> data) {

  base::PlatformFileError error = GDataToPlatformError(status);
  if (error != base::PLATFORM_FILE_OK) {
    callback.Run(error, -1, -1);
    return;
  }

  scoped_ptr<AccountMetadataFeed> feed;
  if (data.get())
      feed.reset(AccountMetadataFeed::CreateFrom(data.get()));
  if (!feed.get()) {
    callback.Run(base::PLATFORM_FILE_ERROR_FAILED, -1, -1);
    return;
  }

  SaveFeed(data.Pass(), FilePath(kAccountMetadataFile));

  callback.Run(base::PLATFORM_FILE_OK,
               feed->quota_bytes_total(),
               feed->quota_bytes_used());
}

std::vector<GDataOperationRegistry::ProgressStatus>
    GDataFileSystem::GetProgressStatusList() {
  return documents_service_->operation_registry()->GetProgressStatusList();
}

bool GDataFileSystem::CancelOperation(const FilePath& file_path) {
  return documents_service_->operation_registry()->CancelForFilePath(file_path);
}

void GDataFileSystem::AddOperationObserver(
    GDataOperationRegistry::Observer* observer) {
  return documents_service_->operation_registry()->AddObserver(observer);
}

void GDataFileSystem::RemoveOperationObserver(
    GDataOperationRegistry::Observer* observer) {
  return documents_service_->operation_registry()->RemoveObserver(observer);
}


void GDataFileSystem::OnCreateDirectoryCompleted(
    const CreateDirectoryParams& params,
    GDataErrorCode status,
    scoped_ptr<base::Value> data) {

  base::PlatformFileError error = GDataToPlatformError(status);
  if (error != base::PLATFORM_FILE_OK) {
    if (!params.callback.is_null())
      params.callback.Run(error);

    return;
  }

  base::DictionaryValue* dict_value = NULL;
  base::Value* created_entry = NULL;
  if (data.get() && data->GetAsDictionary(&dict_value) && dict_value)
    dict_value->Get("entry", &created_entry);
  error = AddNewDirectory(params.created_directory_path, created_entry);

  if (error != base::PLATFORM_FILE_OK) {
    if (!params.callback.is_null())
      params.callback.Run(error);

    return;
  }

  // Not done yet with recursive directory creation?
  if (params.target_directory_path != params.created_directory_path &&
      params.is_recursive) {
    CreateDirectory(params.target_directory_path,
                    params.is_exclusive,
                    params.is_recursive,
                    params.callback);
    return;
  }

  if (!params.callback.is_null()) {
    // Finally done with the create request.
    params.callback.Run(base::PLATFORM_FILE_OK);
  }
}

void GDataFileSystem::OnGetDocuments(
    const FilePath& search_file_path,
    scoped_ptr<ListValue> feed_list,
    scoped_refptr<base::MessageLoopProxy> proxy,
    const FindFileCallback& callback,
    GDataErrorCode status,
    scoped_ptr<base::Value> data) {
  base::PlatformFileError error = GDataToPlatformError(status);
  if (error == base::PLATFORM_FILE_OK &&
      (!data.get() || data->GetType() != Value::TYPE_DICTIONARY)) {
    LOG(WARNING) << "No feed content!";
    error = base::PLATFORM_FILE_ERROR_FAILED;
  }

  if (error != base::PLATFORM_FILE_OK) {
    if (!callback.is_null()) {
      proxy->PostTask(FROM_HERE,
           base::Bind(callback, error, FilePath(),
               reinterpret_cast<GDataFileBase*>(NULL)));
    }

    return;
  }

  // TODO(zelidrag): Find a faster way to get next url rather than parsing
  // the entire feed.
  GURL next_feed_url;
  scoped_ptr<DocumentFeed> current_feed(ParseDocumentFeed(data.get()));
  if (!current_feed.get()) {
    if (!callback.is_null()) {
      proxy->PostTask(FROM_HERE,
           base::Bind(callback, base::PLATFORM_FILE_ERROR_FAILED, FilePath(),
               reinterpret_cast<GDataFileBase*>(NULL)));
    }

    return;
  }

  // Add the current feed to the list of collected feeds for this directory.
  feed_list->Append(data.release());

  // Check if we need to collect more data to complete the directory list.
  if (current_feed->GetNextFeedURL(&next_feed_url) &&
      !next_feed_url.is_empty()) {
    // Kick of the remaining part of the feeds.
    documents_service_->GetDocuments(
        next_feed_url,
        base::Bind(&GDataFileSystem::OnGetDocuments,
                   GetWeakPtrForCurrentThread(),
                   search_file_path,
                   base::Passed(&feed_list),
                   proxy,
                   callback));
    return;
  }

  error = UpdateDirectoryWithDocumentFeed(feed_list.get(), FROM_SERVER);
  if (error != base::PLATFORM_FILE_OK) {
    if (!callback.is_null()) {
      proxy->PostTask(FROM_HERE,
           base::Bind(callback, error, FilePath(),
               reinterpret_cast<GDataFileBase*>(NULL)));
    }

    return;
  }

  scoped_ptr<base::Value> feed_list_value(feed_list.release());
  SaveFeed(feed_list_value.Pass(), FilePath(kLastFeedFile));

  // If we had someone to report this too, then this retrieval was done in a
  // context of search... so continue search.
  if (!callback.is_null()) {
    proxy->PostTask(FROM_HERE,
                    base::Bind(&GDataFileSystem::FindFileByPathOnCallingThread,
                               GetWeakPtrForCurrentThread(),
                               search_file_path,
                               callback));
  }
}

void GDataFileSystem::LoadRootFeedFromCache(
    const FilePath& search_file_path,
    bool load_from_server,
    scoped_refptr<base::MessageLoopProxy> proxy,
    const FindFileCallback& callback) {
  BrowserThread::PostBlockingPoolTask(FROM_HERE,
      base::Bind(&GDataFileSystem::LoadRootFeedOnIOThreadPool,
                 cache_paths_[GDataRootDirectory::CACHE_TYPE_META].Append(
                     kLastFeedFile),
                 proxy,
                 base::Bind(&GDataFileSystem::OnLoadRootFeed,
                            GetWeakPtrForCurrentThread(),
                            search_file_path,
                            load_from_server,
                            proxy,
                            callback)));
}

void GDataFileSystem::OnLoadRootFeed(
    const FilePath& search_file_path,
    bool load_from_server,
    scoped_refptr<base::MessageLoopProxy> proxy,
    FindFileCallback callback,
    base::PlatformFileError error,
    scoped_ptr<base::Value> feed_list) {
  if (error == base::PLATFORM_FILE_OK &&
      (!feed_list.get() || feed_list->GetType() != Value::TYPE_LIST)) {
    LOG(WARNING) << "No feed content!";
    error = base::PLATFORM_FILE_ERROR_FAILED;
  }

  if (error == base::PLATFORM_FILE_OK) {
    error = UpdateDirectoryWithDocumentFeed(
        reinterpret_cast<ListValue*>(feed_list.get()),
        FROM_CACHE);
  }

  // If we got feed content from cache, try search over it.
  if (!load_from_server ||
      (error == base::PLATFORM_FILE_OK && !callback.is_null())) {
    // Continue file content search operation if the delegate hasn't terminated
    // this search branch already.
    proxy->PostTask(FROM_HERE,
                    base::Bind(&GDataFileSystem::FindFileByPathOnCallingThread,
                               GetWeakPtrForCurrentThread(),
                               search_file_path,
                               callback));
    callback.Reset();
  }

  if (!load_from_server)
    return;

  // Kick of the retreival of the feed from server. If we have previously
  // |reported| to the original callback, then we just need to refresh the
  // content without continuing search upon operation completion.
  scoped_ptr<ListValue> server_feed_list(new ListValue());
  documents_service_->GetDocuments(
      GURL(),      // root feed start
      base::Bind(&GDataFileSystem::OnGetDocuments,
                 GetWeakPtrForCurrentThread(),
                 search_file_path,
                 base::Passed(&server_feed_list),
                 proxy,
                 callback));
}

// Static.
void GDataFileSystem::LoadRootFeedOnIOThreadPool(
    const FilePath& file_path,
    scoped_refptr<base::MessageLoopProxy> relay_proxy,
    const GetJsonDocumentCallback& callback) {

  scoped_ptr<base::Value> root_value;
  std::string contents;
  if (!file_util::ReadFileToString(file_path, &contents)) {
    relay_proxy ->PostTask(FROM_HERE,
                           base::Bind(callback,
                                      base::PLATFORM_FILE_ERROR_NOT_FOUND,
                                      base::Passed(&root_value)));
    return;
  }

  int unused_error_code = -1;
  std::string unused_error_message;
  root_value.reset(base::JSONReader::ReadAndReturnError(
      contents, false, &unused_error_code, &unused_error_message));

  bool has_root = root_value.get();
  if (!has_root)
    LOG(WARNING) << "Cached content read failed for file " << file_path.value();

  relay_proxy ->PostTask(FROM_HERE,
      base::Bind(callback,
                 has_root ? base::PLATFORM_FILE_OK :
                            base::PLATFORM_FILE_ERROR_FAILED,
                 base::Passed(&root_value)));
}


void GDataFileSystem::OnFilePathUpdated(const FileOperationCallback& callback,
                                        base::PlatformFileError error,
                                        const FilePath& file_path) {
  if (!callback.is_null())
    callback.Run(error);
}

void GDataFileSystem::OnRenameResourceCompleted(
    const FilePath& file_path,
    const FilePath::StringType& new_name,
    const FilePathUpdateCallback& callback,
    GDataErrorCode status,
    const GURL& document_url) {
  FilePath updated_file_path;
  base::PlatformFileError error = GDataToPlatformError(status);
  if (error == base::PLATFORM_FILE_OK)
    error = RenameFileOnFilesystem(file_path, new_name, &updated_file_path);

  if (!callback.is_null())
    callback.Run(error, updated_file_path);
}

void GDataFileSystem::OnCopyDocumentCompleted(
    const FilePathUpdateCallback& callback,
    GDataErrorCode status,
    scoped_ptr<base::Value> data) {
  base::PlatformFileError error = GDataToPlatformError(status);
  if (error != base::PLATFORM_FILE_OK) {
    if (!callback.is_null())
      callback.Run(error, FilePath());

    return;
  }

  base::DictionaryValue* dict_value = NULL;
  base::Value* entry_value = NULL;
  if (data.get() && data->GetAsDictionary(&dict_value) && dict_value)
    dict_value->Get("entry", &entry_value);

  if (!entry_value) {
    if (!callback.is_null())
      callback.Run(base::PLATFORM_FILE_ERROR_FAILED, FilePath());

    return;
  }

  scoped_ptr<DocumentEntry> entry(DocumentEntry::CreateFrom(entry_value));
  if (!entry.get()) {
    if (!callback.is_null())
      callback.Run(base::PLATFORM_FILE_ERROR_FAILED, FilePath());

    return;
  }

  FilePath file_path;
  {
    base::AutoLock lock(lock_);
    GDataFileBase* file =
        GDataFileBase::FromDocumentEntry(root_.get(), entry.get(), root_.get());
    if (!file) {
      if (!callback.is_null())
        callback.Run(base::PLATFORM_FILE_ERROR_FAILED, FilePath());

      return;
    }
    root_->AddFile(file);
    file_path = file->GetFilePath();
  }

  NotifyDirectoryChanged(file_path.DirName());

  if (!callback.is_null())
    callback.Run(error, file_path);
}

void GDataFileSystem::OnAddFileToDirectoryCompleted(
    const FileOperationCallback& callback,
    const FilePath& file_path,
    const FilePath& dir_path,
    GDataErrorCode status,
    const GURL& document_url) {
  base::PlatformFileError error = GDataToPlatformError(status);
  if (error == base::PLATFORM_FILE_OK)
    error = AddFileToDirectoryOnFilesystem(file_path, dir_path);

  if (!callback.is_null())
    callback.Run(error);
}

void GDataFileSystem::OnRemoveFileFromDirectoryCompleted(
    const FilePathUpdateCallback& callback,
    const FilePath& file_path,
    const FilePath& dir_path,
    GDataErrorCode status,
    const GURL& document_url) {
  FilePath updated_file_path = file_path;
  base::PlatformFileError error = GDataToPlatformError(status);
  if (error == base::PLATFORM_FILE_OK)
    error = RemoveFileFromDirectoryOnFilesystem(file_path, dir_path,
                                                &updated_file_path);

  if (!callback.is_null())
    callback.Run(error, updated_file_path);
}

void GDataFileSystem::SaveFeed(scoped_ptr<base::Value> feed,
                               const FilePath& name) {
  BrowserThread::PostBlockingPoolSequencedTask(kGDataFileSystemToken, FROM_HERE,
      base::Bind(&GDataFileSystem::SaveFeedOnIOThreadPool,
                 cache_paths_[GDataRootDirectory::CACHE_TYPE_META],
                 base::Passed(&feed),
                 name));
}

// Static.
void GDataFileSystem::SaveFeedOnIOThreadPool(
    const FilePath& meta_cache_path,
    scoped_ptr<base::Value> feed,
    const FilePath& name) {
  if (!file_util::DirectoryExists(meta_cache_path)) {
    if (!file_util::CreateDirectory(meta_cache_path)) {
      LOG(WARNING) << "GData metadata cache directory can't be created at "
                   << meta_cache_path.value();
      return;
    }
  }

  FilePath file_name = meta_cache_path.Append(name);
  std::string json;
  base::JSONWriter::Write(feed.get(), &json);

  int file_size = static_cast<int>(json.length());
  if (file_util::WriteFile(file_name, json.data(), file_size) != file_size) {
    LOG(WARNING) << "GData metadata file can't be stored at "
                 << file_name.value();
    if (!file_util::Delete(file_name, true)) {
      LOG(WARNING) << "GData metadata file can't be deleted at "
                   << file_name.value();
      return;
    }
  }
}

void GDataFileSystem::OnRemovedDocument(
    const FileOperationCallback& callback,
    const FilePath& file_path,
    GDataErrorCode status,
    const GURL& document_url) {
  base::PlatformFileError error = GDataToPlatformError(status);

  if (error == base::PLATFORM_FILE_OK)
    error = RemoveFileFromFileSystem(file_path);

  if (!callback.is_null()) {
    callback.Run(error);
  }
}

void GDataFileSystem::OnFileDownloaded(
    const GetFileFromCacheParams& params,
    GDataErrorCode status,
    const GURL& content_url,
    const FilePath& downloaded_file_path) {
  base::PlatformFileError error = GDataToPlatformError(status);

  // Make sure that downloaded file is properly stored in cache. We don't have
  // to wait for this operation to finish since the user can already use the
  // downloaded file.
  if (error == base::PLATFORM_FILE_OK) {
    StoreToCache(params.resource_id,
                 params.md5,
                 downloaded_file_path,
                 base::Bind(&GDataFileSystem::OnDownloadStoredToCache,
                            GetWeakPtrForCurrentThread()));
  }

  if (!params.callback.is_null()) {
    params.proxy->PostTask(FROM_HERE,
                           base::Bind(params.callback,
                                      error,
                                      downloaded_file_path,
                                      REGULAR_FILE));
  }
}

void GDataFileSystem::OnDownloadStoredToCache(base::PlatformFileError error,
                                              const std::string& resource_id,
                                              const std::string& md5) {
  // Nothing much to do here for now.
}

base::PlatformFileError GDataFileSystem::RenameFileOnFilesystem(
    const FilePath& file_path,
    const FilePath::StringType& new_name,
    FilePath* updated_file_path) {
  DCHECK(updated_file_path);

  base::AutoLock lock(lock_);
  GDataFileBase* file = GetGDataFileInfoFromPath(file_path);
  if (!file)
    return base::PLATFORM_FILE_ERROR_NOT_FOUND;

  DCHECK(file->parent());
  file->set_title(new_name);
  // After changing the title of the file, call TakeFile() to remove the
  // file from its parent directory and then add it back in order to go
  // through the file name de-duplication.
  if (!file->parent()->TakeFile(file))
    return base::PLATFORM_FILE_ERROR_FAILED;

  *updated_file_path = file->GetFilePath();

  NotifyDirectoryChanged(updated_file_path->DirName());
  return base::PLATFORM_FILE_OK;
}

base::PlatformFileError GDataFileSystem::AddFileToDirectoryOnFilesystem(
    const FilePath& file_path, const FilePath& dir_path) {
  base::AutoLock lock(lock_);
  GDataFileBase* file = GetGDataFileInfoFromPath(file_path);
  if (!file)
    return base::PLATFORM_FILE_ERROR_NOT_FOUND;

  DCHECK_EQ(root_.get(), file->parent());

  GDataFileBase* dir_file = GetGDataFileInfoFromPath(dir_path);
  if (!dir_file)
    return base::PLATFORM_FILE_ERROR_NOT_FOUND;

  GDataDirectory* dir = dir_file->AsGDataDirectory();
  if (!dir)
    return base::PLATFORM_FILE_ERROR_NOT_A_DIRECTORY;

  if (!dir->TakeFile(file))
    return base::PLATFORM_FILE_ERROR_FAILED;

  NotifyDirectoryChanged(dir_path);
  return base::PLATFORM_FILE_OK;
}

base::PlatformFileError GDataFileSystem::RemoveFileFromDirectoryOnFilesystem(
    const FilePath& file_path, const FilePath& dir_path,
    FilePath* updated_file_path) {
  DCHECK(updated_file_path);

  base::AutoLock lock(lock_);
  GDataFileBase* file = GetGDataFileInfoFromPath(file_path);
  if (!file)
    return base::PLATFORM_FILE_ERROR_NOT_FOUND;

  GDataFileBase* dir = GetGDataFileInfoFromPath(dir_path);
  if (!dir)
    return base::PLATFORM_FILE_ERROR_NOT_FOUND;

  if (!dir->AsGDataDirectory())
    return base::PLATFORM_FILE_ERROR_NOT_A_DIRECTORY;

  DCHECK_EQ(dir->AsGDataDirectory(), file->parent());

  if (!root_->TakeFile(file))
    return base::PLATFORM_FILE_ERROR_FAILED;

  *updated_file_path = file->GetFilePath();

  NotifyDirectoryChanged(updated_file_path->DirName());
  return base::PLATFORM_FILE_OK;
}

base::PlatformFileError GDataFileSystem::RemoveFileFromFileSystem(
    const FilePath& file_path) {

  std::string resource_id;
  base::PlatformFileError error = RemoveFileFromGData(file_path, &resource_id);
  if (error != base::PLATFORM_FILE_OK)
    return error;

  // If resource_id is not empty, remove its corresponding file from cache.
  if (!resource_id.empty())
    RemoveFromCache(resource_id, CacheOperationCallback());

  return base::PLATFORM_FILE_OK;
}

DocumentFeed* GDataFileSystem::ParseDocumentFeed(base::Value* feed_data) {
  DCHECK(feed_data->IsType(Value::TYPE_DICTIONARY));
  base::DictionaryValue* feed_dict = NULL;
  if (!static_cast<base::DictionaryValue*>(feed_data)->GetDictionary(
          kFeedField, &feed_dict)) {
    return NULL;
  }
  // Parse the document feed.
  return DocumentFeed::CreateFrom(feed_dict);
}

base::PlatformFileError GDataFileSystem::UpdateDirectoryWithDocumentFeed(
    ListValue* feed_list,
    ContentOrigin origin) {
  DVLOG(1) << "Updating directory with feed from "
           << (origin == FROM_CACHE ? "cache" : "web server");

  // We need to lock here as well (despite FindFileByPath lock) since directory
  // instance below is a 'live' object.
  base::AutoLock lock(lock_);
  // Don't send directory content change notification while performing
  // the initial content retreival.
  bool should_nofify = root_->origin() != UNINITIALIZED;

  root_->set_origin(origin);
  root_->set_refresh_time(base::Time::Now());
  root_->RemoveChildren();

  // A map of self URLs to pairs of gdata file and parent URL.
  typedef std::map<GURL, std::pair<GDataFileBase*, GURL> >
      UrlToFileAndParentMap;
  UrlToFileAndParentMap file_by_url;
  bool first_feed = true;
  base::PlatformFileError error = base::PLATFORM_FILE_OK;

  for (ListValue::const_iterator feeds_iter = feed_list->begin();
       feeds_iter != feed_list->end(); ++feeds_iter) {
    scoped_ptr<DocumentFeed> feed(ParseDocumentFeed(*feeds_iter));
    if (!feed.get()) {
      error = base::PLATFORM_FILE_ERROR_FAILED;
      break;
    }

    // Get upload url from the root feed. Links for all other collections will
    // be handled in GDatadirectory::FromDocumentEntry();
    if (first_feed) {
      const Link* root_feed_upload_link =
          feed->GetLinkByType(Link::RESUMABLE_CREATE_MEDIA);
      if (root_feed_upload_link)
        root_->set_upload_url(root_feed_upload_link->href());
      first_feed = false;
    }

    for (ScopedVector<DocumentEntry>::const_iterator iter =
             feed->entries().begin();
         iter != feed->entries().end(); ++iter) {
      DocumentEntry* doc = *iter;
      GDataFileBase* file = GDataFileBase::FromDocumentEntry(NULL, doc,
                                                             root_.get());
      // Some document entries don't map into files (i.e. sites).
      if (!file)
        continue;

      GURL parent_url;
      const Link* parent_link = doc->GetLinkByType(Link::PARENT);
      if (parent_link)
        parent_url = parent_link->href();

      UrlToFileAndParentMap::mapped_type& map_entry =
          file_by_url[file->self_url()];
      // An entry with the same self link may already exist, so we need to
      // release the existing GDataFileBase instance before overwriting the
      // entry with another GDataFileBase instance.
      delete map_entry.first;
      map_entry.first = file;
      map_entry.second = parent_url;
    }
  }

  if (error != base::PLATFORM_FILE_OK) {
    // If the code above fails to parse a feed, any GDataFileBase instance
    // added to |file_by_url| is not managed by a GDataDirectory instance,
    // so we need to explicitly release them here.
    for (UrlToFileAndParentMap::iterator it = file_by_url.begin();
         it != file_by_url.end(); ++it) {
      delete it->second.first;
    }
    return error;
  }

  for (UrlToFileAndParentMap::iterator it = file_by_url.begin();
       it != file_by_url.end(); ++it) {
    scoped_ptr<GDataFileBase> file(it->second.first);
    GURL parent_url = it->second.second;
    GDataDirectory* dir = root_.get();
    if (!parent_url.is_empty()) {
      UrlToFileAndParentMap::iterator find_iter = file_by_url.find(parent_url);
      if (find_iter == file_by_url.end()) {
        LOG(WARNING) << "Found orphaned file '" << file->file_name()
                     << "' with non-existing parent folder of "
                     << parent_url.spec();
      } else {
        dir = find_iter->second.first->AsGDataDirectory();
        if (!dir) {
          LOG(WARNING) << "Found orphaned file '" << file->file_name()
                       << "' pointing to non directory parent "
                       << parent_url.spec();
          dir = root_.get();
        }
      }
    }
    DCHECK(dir);

    dir->AddFile(file.release());
  }

  if (should_nofify)
    NotifyDirectoryChanged(root_->GetFilePath());

  return base::PLATFORM_FILE_OK;
}

void GDataFileSystem::NotifyCacheInitialized() {
  DVLOG(1) << "Cache initialized";
  if (!BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    BrowserThread::PostTask(
        BrowserThread::UI,
        FROM_HERE,
        base::Bind(&GDataFileSystem::NotifyCacheInitialized,
                   ui_weak_ptr_));
    return;
  }
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  // Notify the observers that the cache is initialized.
  FOR_EACH_OBSERVER(Observer, observers_, OnCacheInitialized());
}

void GDataFileSystem::NotifyFilePinned(const std::string& resource_id,
                                       const std::string& md5) {
  DVLOG(1) << "File pinned " << resource_id << ": " << md5;
  if (!BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    BrowserThread::PostTask(
        BrowserThread::UI,
        FROM_HERE,
        base::Bind(&GDataFileSystem::NotifyFilePinned,
                   ui_weak_ptr_,
                   resource_id,
                   md5));
    return;
  }
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  // Notify the observers that a file is pinned with |resource_id| and |md5|.
  FOR_EACH_OBSERVER(Observer, observers_, OnFilePinned(resource_id, md5));
}

void GDataFileSystem::NotifyFileUnpinned(const std::string& resource_id,
                                         const std::string& md5) {
  DVLOG(1) << "File unpinned " << resource_id << ": " << md5;
  if (!BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    BrowserThread::PostTask(
        BrowserThread::UI,
        FROM_HERE,
        base::Bind(&GDataFileSystem::NotifyFileUnpinned,
                   ui_weak_ptr_,
                   resource_id,
                   md5));
    return;
  }
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  // Notify the observers that a file is unpinned with |resource_id| and |md5|.
  FOR_EACH_OBSERVER(Observer, observers_, OnFileUnpinned(resource_id, md5));
}

void GDataFileSystem::NotifyDirectoryChanged(const FilePath& directory_path) {
  DVLOG(1) << "Content changed of " << directory_path.value();
  if (!BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    BrowserThread::PostTask(
        BrowserThread::UI,
        FROM_HERE,
        base::Bind(&GDataFileSystem::NotifyDirectoryChanged,
                   ui_weak_ptr_,
                   directory_path));
    return;
  }
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  // Notify the observers that content of |directory_path| has been changed.
  FOR_EACH_OBSERVER(Observer, observers_, OnDirectoryChanged(directory_path));
}

base::PlatformFileError GDataFileSystem::AddNewDirectory(
    const FilePath& directory_path, base::Value* entry_value) {
  if (!entry_value)
    return base::PLATFORM_FILE_ERROR_FAILED;

  scoped_ptr<DocumentEntry> entry(DocumentEntry::CreateFrom(entry_value));

  if (!entry.get())
    return base::PLATFORM_FILE_ERROR_FAILED;

  // We need to lock here as well (despite FindFileByPath lock) since directory
  // instance below is a 'live' object.
  base::AutoLock lock(lock_);

  // Find parent directory element within the cached file system snapshot.
  GDataFileBase* file = GetGDataFileInfoFromPath(directory_path);
  if (!file)
    return base::PLATFORM_FILE_ERROR_FAILED;

  // Check if parent is a directory since in theory since this is a callback
  // something could in the meantime have nuked the parent dir and created a
  // file with the exact same name.
  GDataDirectory* parent_dir = file->AsGDataDirectory();
  if (!parent_dir)
    return base::PLATFORM_FILE_ERROR_FAILED;

  GDataFileBase* new_file = GDataFileBase::FromDocumentEntry(parent_dir,
                                                             entry.get(),
                                                             root_.get());
  if (!new_file)
    return base::PLATFORM_FILE_ERROR_FAILED;

  parent_dir->AddFile(new_file);

  NotifyDirectoryChanged(directory_path);
  return base::PLATFORM_FILE_OK;
}

GDataFileSystem::FindMissingDirectoryResult
GDataFileSystem::FindFirstMissingParentDirectory(
    const FilePath& directory_path,
    GURL* last_dir_content_url,
    FilePath* first_missing_parent_path) {
  // Let's find which how deep is the existing directory structure and
  // get the first element that's missing.
  std::vector<FilePath::StringType> path_parts;
  directory_path.GetComponents(&path_parts);
  FilePath current_path;

  base::AutoLock lock(lock_);
  for (std::vector<FilePath::StringType>::const_iterator iter =
          path_parts.begin();
       iter != path_parts.end(); ++iter) {
    current_path = current_path.Append(*iter);
    GDataFileBase* file = GetGDataFileInfoFromPath(current_path);
    if (file) {
      if (file->file_info().is_directory) {
        *last_dir_content_url = file->content_url();
      } else {
        // Huh, the segment found is a file not a directory?
        return FOUND_INVALID;
      }
    } else {
      *first_missing_parent_path = current_path;
      return FOUND_MISSING;
    }
  }
  return DIRECTORY_ALREADY_PRESENT;
}

GURL GDataFileSystem::GetUploadUrlForDirectory(
    const FilePath& destination_directory) {
  // Find directory element within the cached file system snapshot.
  base::AutoLock lock(lock_);
  GDataFileBase* file = GetGDataFileInfoFromPath(destination_directory);
  GDataDirectory* dir = file ? file->AsGDataDirectory() : NULL;
  return dir ? dir->upload_url() : GURL();
}

base::PlatformFileError GDataFileSystem::RemoveFileFromGData(
    const FilePath& file_path, std::string* resource_id) {
  resource_id->clear();

  // We need to lock here as well (despite FindFileByPath lock) since
  // directory instance below is a 'live' object.
  base::AutoLock lock(lock_);

  // Find directory element within the cached file system snapshot.
  GDataFileBase* file = GetGDataFileInfoFromPath(file_path);

  if (!file)
    return base::PLATFORM_FILE_ERROR_NOT_FOUND;

  // You can't remove root element.
  if (!file->parent())
    return base::PLATFORM_FILE_ERROR_ACCESS_DENIED;

  // If it's a file (only files have resource id), get its resource id so that
  // we can remove it after releasing the auto lock.
  if (file->AsGDataFile())
    *resource_id = file->AsGDataFile()->resource_id();

  GDataDirectory* parent_dir = file->parent();
  if (!parent_dir->RemoveFile(file))
    return base::PLATFORM_FILE_ERROR_NOT_FOUND;

  NotifyDirectoryChanged(parent_dir->GetFilePath());
  return base::PLATFORM_FILE_OK;
}

void GDataFileSystem::AddDownloadedFile(const FilePath& virtual_dir_path,
                                        scoped_ptr<gdata::DocumentEntry> entry,
                                        const FilePath& file_content_path) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  if (!entry.get()) {
    NOTREACHED();
    return;
  }

  std::string resource_id;
  std::string md5;
  {
    base::AutoLock lock(lock_);
    GDataFileBase* dir_file = GetGDataFileInfoFromPath(virtual_dir_path);
    if (!dir_file)
      return;

    GDataDirectory* parent_dir  = dir_file->AsGDataDirectory();
    if (!parent_dir)
      return;

    scoped_ptr<GDataFileBase> new_file(
        GDataFileBase::FromDocumentEntry(parent_dir, entry.get(), root_.get()));
    if (!new_file.get())
      return;

    GDataFile* file = new_file->AsGDataFile();
    DCHECK(file);
    resource_id = file->resource_id();
    md5 = file->file_md5();
    parent_dir->AddFile(new_file.release());
  }
  NotifyDirectoryChanged(virtual_dir_path);
  StoreToCache(resource_id, md5, file_content_path, CacheOperationCallback());
}


//===================== GDataFileSystem: Cache entry points ====================

FilePath GDataFileSystem::GetCacheFilePath(
    const std::string& resource_id,
    const std::string& md5,
    GDataRootDirectory::CacheSubDirectoryType sub_dir_type,
    CachedFileOrigin file_origin) const {
  DCHECK(sub_dir_type != GDataRootDirectory::CACHE_TYPE_META);

  // Runs on any thread.
  // Filename is formatted as resource_id.md5, i.e. resource_id is the base
  // name and md5 is the extension.
  std::string base_name = GDataFileBase::EscapeUtf8FileName(resource_id);
  if (file_origin == CACHED_FILE_LOCALLY_MODIFIED) {
    DCHECK(sub_dir_type == GDataRootDirectory::CACHE_TYPE_PERSISTENT);
    base_name += FilePath::kExtensionSeparator;
    base_name += kLocallyModifiedFileExtension;
  } else if (!md5.empty()) {
    base_name += FilePath::kExtensionSeparator;
    base_name += GDataFileBase::EscapeUtf8FileName(md5);
  }
  return cache_paths_[sub_dir_type].Append(base_name);
}

void GDataFileSystem::GetFromCache(const std::string& resource_id,
                                   const std::string& md5,
                                   const GetFromCacheCallback& callback) {
  GetFromCacheInternal(resource_id, md5, FilePath(), callback);
}

void GDataFileSystem::StoreToCache(const std::string& resource_id,
                                   const std::string& md5,
                                   const FilePath& source_path,
                                   const CacheOperationCallback& callback) {
  InitializeCacheIfNecessary();

  PostBlockingPoolSequencedTask(
      kGDataFileSystemToken,
      FROM_HERE,
      base::Bind(&GDataFileSystem::StoreToCacheOnIOThreadPool,
                 base::Unretained(this),
                 ModifyCacheStateParams(
                     resource_id,
                     md5,
                     source_path,
                     callback,
                     CacheOperationIntermediateCallback(),
                     base::MessageLoopProxy::current())));
}

void GDataFileSystem::Pin(const std::string& resource_id,
                          const std::string& md5,
                          const CacheOperationCallback& callback) {
  InitializeCacheIfNecessary();

  PostBlockingPoolSequencedTask(
      kGDataFileSystemToken,
      FROM_HERE,
      base::Bind(&GDataFileSystem::PinOnIOThreadPool,
                 base::Unretained(this),
                 ModifyCacheStateParams(
                     resource_id,
                     md5,
                     FilePath(),  // |source_path| is not used.
                     callback,
                     base::Bind(&GDataFileSystem::OnFilePinned,
                                GetWeakPtrForCurrentThread()),
                     base::MessageLoopProxy::current())));
}

void GDataFileSystem::Unpin(const std::string& resource_id,
                            const std::string& md5,
                            const CacheOperationCallback& callback) {
  InitializeCacheIfNecessary();

  PostBlockingPoolSequencedTask(
      kGDataFileSystemToken,
      FROM_HERE,
      base::Bind(&GDataFileSystem::UnpinOnIOThreadPool,
                 base::Unretained(this),
                 ModifyCacheStateParams(
                     resource_id,
                     md5,
                     FilePath(),  // |source_path| is not used.
                     callback,
                     base::Bind(&GDataFileSystem::OnFileUnpinned,
                                GetWeakPtrForCurrentThread()),
                     base::MessageLoopProxy::current())));
}

void GDataFileSystem::RemoveFromCache(const std::string& resource_id,
                                      const CacheOperationCallback& callback) {
  InitializeCacheIfNecessary();

  PostBlockingPoolSequencedTask(
      kGDataFileSystemToken,
      FROM_HERE,
      base::Bind(&GDataFileSystem::RemoveFromCacheOnIOThreadPool,
                 base::Unretained(this),
                 resource_id,
                 callback,
                 base::MessageLoopProxy::current()));
}

void GDataFileSystem::InitializeCacheIfNecessary() {
  // Lock to access cache_initialized_started_;
  base::AutoLock lock(lock_);
  UnsafeInitializeCacheIfNecessary();
}

//========= GDataFileSystem: Cache tasks that ran on io thread pool ============

void GDataFileSystem::InitializeCacheOnIOThreadPool() {
  base::PlatformFileError error = CreateCacheDirectories(cache_paths_);

  if (error != base::PLATFORM_FILE_OK)
    return;

  // Scan cache persistent and tmp directories to enumerate all files and create
  // corresponding entries for cache map.
  GDataRootDirectory::CacheMap cache_map;
  ScanCacheDirectory(GDataRootDirectory::CACHE_TYPE_PERSISTENT, &cache_map);
  ScanCacheDirectory(GDataRootDirectory::CACHE_TYPE_TMP, &cache_map);

  // Then scan pinned directory to update existing entries in cache map, or
  // create new ones for symlinks to /dev/null which target nothing.
  // Pinned directory should be scanned after the persistent directory as we'll
  // add PINNED state to the existing files in the persistent directory per the
  // contents of the pinned directory.
  ScanCacheDirectory(GDataRootDirectory::CACHE_TYPE_PINNED, &cache_map);

  // Lock to update cache map.
  base::AutoLock lock(lock_);
  root_->SetCacheMap(cache_map);

  NotifyCacheInitialized();
}

void GDataFileSystem::GetFromCacheOnIOThreadPool(
    const std::string& resource_id,
    const std::string& md5,
    const FilePath& gdata_file_path,
    const GetFromCacheCallback& callback,
    scoped_refptr<base::MessageLoopProxy> relay_proxy) {
  // Lock to access cache map.
  base::AutoLock lock(lock_);

  base::PlatformFileError error = base::PLATFORM_FILE_OK;
  FilePath cache_file_path;

  GDataRootDirectory::CacheEntry* entry = root_->GetCacheEntry(resource_id,
                                                               md5);

  if (entry && entry->cache_state & GDataFile::CACHE_STATE_PRESENT) {
    cache_file_path = GetCacheFilePath(resource_id,
                                       md5,
                                       entry->sub_dir_type,
                                       CACHED_FILE_FROM_SERVER);
  } else {
    error = base::PLATFORM_FILE_ERROR_NOT_FOUND;
  }

  // Invoke |callback|.
  relay_proxy->PostTask(FROM_HERE,
                        base::Bind(callback,
                                   error,
                                   resource_id,
                                   md5,
                                   gdata_file_path,
                                   cache_file_path));
}

void GDataFileSystem::GetCacheStateOnIOThreadPool(
    const std::string& resource_id,
    const std::string& md5,
    const GetCacheStateCallback& final_callback,
    const GetCacheStateIntermediateCallback& intermediate_callback,
    scoped_refptr<base::MessageLoopProxy> relay_proxy) {
  // Lock to access cache map.
  base::AutoLock lock(lock_);

  base::PlatformFileError error = base::PLATFORM_FILE_OK;
  int cache_state = GDataFile::CACHE_STATE_NONE;

  // Get file object for |resource_id|.
  GDataFileBase* file_base = root_->GetFileByResource(resource_id);
  GDataFile* file = NULL;
  if (!file_base || !file_base->AsGDataFile()) {
    error = base::PLATFORM_FILE_ERROR_NOT_FOUND;
  } else {
    file = file_base->AsGDataFile();
    // Get cache state of file corresponding to |resource_id| and |md5|.
    GDataRootDirectory::CacheEntry* entry = root_->GetCacheEntry(resource_id,
                                                                 md5);
    if (entry)
      cache_state = entry->cache_state;
  }

  // Invoke |callback|.
  relay_proxy->PostTask(FROM_HERE,
                        base::Bind(intermediate_callback,
                                   error,
                                   file,
                                   cache_state,
                                   final_callback));
}

void GDataFileSystem::StoreToCacheOnIOThreadPool(
    const ModifyCacheStateParams& params) {
  // Lock to access cache map.
  base::AutoLock lock(lock_);

  FilePath dest_path;
  FilePath symlink_path;
  int cache_state = GDataFile::CACHE_STATE_PRESENT;
  GDataRootDirectory::CacheSubDirectoryType sub_dir_type =
      GDataRootDirectory::CACHE_TYPE_TMP;

  GDataRootDirectory::CacheEntry* entry = root_->GetCacheEntry(
      params.resource_id, params.md5);

  // If file was previously pinned, store it in persistent dir and create
  // symlink in pinned dir.
  if (entry) {  // File exists in cache, determines destination path.
    cache_state |= entry->cache_state;
    if (entry->cache_state & GDataFile::CACHE_STATE_PINNED) {
      sub_dir_type = GDataRootDirectory::CACHE_TYPE_PERSISTENT;
      dest_path = GetCacheFilePath(params.resource_id, params.md5, sub_dir_type,
                                   CACHED_FILE_FROM_SERVER);
      symlink_path = GetCacheFilePath(params.resource_id, std::string(),
                                      GDataRootDirectory::CACHE_TYPE_PINNED,
                                      CACHED_FILE_FROM_SERVER);
    }
  }

  // File wasn't pinned or doesn't exist in cache, store in tmp dir.
  if (dest_path.empty()) {
    DCHECK_EQ(GDataRootDirectory::CACHE_TYPE_TMP, sub_dir_type);
    dest_path = GetCacheFilePath(params.resource_id, params.md5, sub_dir_type,
                                 CACHED_FILE_FROM_SERVER);
  }

  base::PlatformFileError error = ModifyCacheState(
      params.source_path,
      dest_path,
      symlink_path,
      !symlink_path.empty());  // create symlink

  // Determine search pattern for stale filenames corrresponding to resource_id,
  // either "<resource_id>*" or "<resource_id>.*".
  FilePath stale_filenames_pattern;
  if (params.md5.empty()) {
    // No md5 means no extension, append '*' after base name, i.e.
    // "<resource_id>*".
    // Cannot call |dest_path|.ReplaceExtension when there's no md5 extension:
    // if base name of |dest_path| (i.e. escaped resource_id) contains the
    // extension separator '.', ReplaceExtension will remove it and everything
    // after it.  The result will be nothing like the escaped resource_id.
    stale_filenames_pattern = FilePath(dest_path.value() + kWildCard);
  } else {
    // Replace md5 extension with '*' i.e. "<resource_id>.*".
    // Note that ReplaceExtension automatically prefixes the extension with the
    // extension separator '.'.
    stale_filenames_pattern = dest_path.ReplaceExtension(kWildCard);
  }

  // Delete files that match |stale_filenames_pattern| except for |dest_path|.
  DeleteFilesSelectively(stale_filenames_pattern, dest_path);

  if (error == base::PLATFORM_FILE_OK) {
    // Now that file operations have completed, update cache map.
    root_->UpdateCacheMap(params.resource_id, params.md5, sub_dir_type,
                          cache_state);
  }

  // Invoke |final_callback|.
  if (!params.final_callback.is_null()) {
    params.relay_proxy->PostTask(FROM_HERE,
                                 base::Bind(params.final_callback,
                                            error,
                                            params.resource_id,
                                            params.md5));
  }
}

void GDataFileSystem::PinOnIOThreadPool(const ModifyCacheStateParams& params) {
  // Lock to access cache map.
  base::AutoLock lock(lock_);

  FilePath source_path;
  FilePath dest_path;
  FilePath symlink_path;
  bool create_symlink = true;
  int cache_state = GDataFile::CACHE_STATE_PINNED;
  GDataRootDirectory::CacheSubDirectoryType sub_dir_type =
      GDataRootDirectory::CACHE_TYPE_PERSISTENT;

  GDataRootDirectory::CacheEntry* entry = root_->GetCacheEntry(
      params.resource_id, params.md5);

  if (!entry) {  // Entry does not exist in cache.
    // Set both |dest_path| and |source_path| to /dev/null, so that:
    // 1) ModifyCacheState won't move files when |source_path| and |dest_path|
    //    are the same.
    // 2) symlinks to /dev/null will be picked up by GDataSyncClient to download
    //    pinned files that don't exist in cache.
    dest_path = FilePath(kSymLinkToDevNull);
    source_path = dest_path;

    // Set sub_dir_type to PINNED to indicate that the file doesn't exist.
    // When the file is finally downloaded and StoreToCache called, it will be
    // moved to persistent directory.
    sub_dir_type = GDataRootDirectory::CACHE_TYPE_PINNED;
  } else {  // File exists in cache, determines destination path.
    cache_state |= entry->cache_state;

    // Gets the current path of the file in cache.
    source_path = GetCacheFilePath(params.resource_id,
                                   params.md5,
                                   entry->sub_dir_type,
                                   CACHED_FILE_FROM_SERVER);
    switch (entry->sub_dir_type) {
      case GDataRootDirectory::CACHE_TYPE_PERSISTENT:
        // File is already in persistent dir, set |dest_path| to |source_path|,
        // because ModifyCacheState only moves files if source and destination
        // are different, and we don't need to move the file in this case.
        dest_path = source_path;
        break;
      case GDataRootDirectory::CACHE_TYPE_TMP:
        // File is in tmp dir, move it to persistent dir.
        dest_path = GetCacheFilePath(params.resource_id, params.md5,
                                     GDataRootDirectory::CACHE_TYPE_PERSISTENT,
                                     CACHED_FILE_FROM_SERVER);
        break;
      case GDataRootDirectory::CACHE_TYPE_PINNED:
        // File was pinned before and remain so, actual file blob still doesn't
        // exist in cache, nothing else to do.  Simply set |dest_path| to
        // |source_path| so no move operation will occur, and don't create
        // symlink.
        dest_path = source_path;
        create_symlink = false;
        break;
      default:
        NOTREACHED() <<  "Unexpected sub dir type: " << entry->sub_dir_type;
        // Force no operation by setting |dest_path| to |source_path| and don't
        // create symlink.
        dest_path = source_path;
        create_symlink = false;
        break;
    }
  }

  // Create symlink in pinned dir.
  if (create_symlink) {
    symlink_path = GetCacheFilePath(params.resource_id,
                                    std::string(),
                                    GDataRootDirectory::CACHE_TYPE_PINNED,
                                    CACHED_FILE_FROM_SERVER);
  }

  base::PlatformFileError error = ModifyCacheState(source_path,
                                                   dest_path,
                                                   symlink_path,
                                                   create_symlink);

  if (error == base::PLATFORM_FILE_OK) {
    // Now that file operations have completed, update cache map.
    root_->UpdateCacheMap(params.resource_id, params.md5, sub_dir_type,
                          cache_state);
  }

  // Invoke |intermediate_callback|.
  params.relay_proxy->PostTask(FROM_HERE,
                               base::Bind(params.intermediate_callback,
                                          error,
                                          params.resource_id,
                                          params.md5,
                                          params.final_callback));
}

void GDataFileSystem::UnpinOnIOThreadPool(
    const ModifyCacheStateParams& params) {
  // Lock to access cache map.
  base::AutoLock lock(lock_);

  GDataRootDirectory::CacheEntry* entry = root_->GetCacheEntry(
      params.resource_id, params.md5);

  // Unpinning a file means its entry must exist in cache.
  if (!entry) {
    LOG(WARNING) << "Can't unpin a file that wasn't pinned or cached: res_id="
                 << params.resource_id
                 << ", md5=" << params.md5;
    // Invoke |intermediate_callback|.
    params.relay_proxy->PostTask(FROM_HERE,
                                 base::Bind(params.intermediate_callback,
                                            base::PLATFORM_FILE_ERROR_NOT_FOUND,
                                            params.resource_id,
                                            params.md5,
                                            params.final_callback));
    return;
  }

  // Entry exists in cache, determines destination path.

  FilePath dest_path;
  FilePath symlink_path;

  // Gets the current path of the file in cache.
  FilePath source_path = GetCacheFilePath(params.resource_id,
                                          params.md5,
                                          entry->sub_dir_type,
                                          CACHED_FILE_FROM_SERVER);

  switch (entry->sub_dir_type) {
    case GDataRootDirectory::CACHE_TYPE_PERSISTENT:
      // File is in persistent dir, move it to tmp dir.
      dest_path = GetCacheFilePath(params.resource_id, params.md5,
                                   GDataRootDirectory::CACHE_TYPE_TMP,
                                   CACHED_FILE_FROM_SERVER);
      break;
    case GDataRootDirectory::CACHE_TYPE_TMP:
      // File is already in tmp dir, set |dest_path| to |source_path|,
      // because ModifyCacheState only moves files if source and destination
      // are different.
      dest_path = source_path;
      break;
    case GDataRootDirectory::CACHE_TYPE_PINNED:
      // File was pinned before but actual file blob doesn't exist in cache.
      // Set |dest_path| to |source_path| so no move operation will occur.
      dest_path = source_path;
      break;
    default:
      NOTREACHED() <<  "Unexpected sub dir type: " << entry->sub_dir_type;
      // Force no operation by setting |dest_path| to |source_path|.
      dest_path = source_path;
      break;
  }

  // Get absolute path of symlink in pinned dir so as to remove it.
  if (entry->cache_state & GDataFile::CACHE_STATE_PINNED) {
    symlink_path = GetCacheFilePath(params.resource_id,
                                    std::string(),
                                    GDataRootDirectory::CACHE_TYPE_PINNED,
                                    CACHED_FILE_FROM_SERVER);
  }

  base::PlatformFileError error = ModifyCacheState(
      source_path,
      dest_path,
      symlink_path,  // This will be deleted if it exists.
      false /* don't create symlink*/);

  if (error == base::PLATFORM_FILE_OK) {
    // Now that file operations have completed, update cache map.
    root_->UpdateCacheMap(params.resource_id, params.md5,
                          GDataRootDirectory::CACHE_TYPE_TMP,
                          entry->cache_state & ~GDataFile::CACHE_STATE_PINNED);
  }

  // Invoke |intermediate_callback|.
  params.relay_proxy->PostTask(FROM_HERE,
                               base::Bind(params.intermediate_callback,
                                          error,
                                          params.resource_id,
                                          params.md5,
                                          params.final_callback));
}

void GDataFileSystem::RemoveFromCacheOnIOThreadPool(
    const std::string& resource_id,
    const gdata::CacheOperationCallback& callback,
    scoped_refptr<base::MessageLoopProxy> relay_proxy) {
  // Determine paths to delete all cache versions of |resource_id| in
  // persistent, tmp and pinned directories.
  std::vector<FilePath> paths_to_delete;

  // For files in persistent and tmp dirs, delete files that match
  // "<resource_id>.*".
  paths_to_delete.push_back(GetCacheFilePath(
      resource_id,
      kWildCard,
      GDataRootDirectory::CACHE_TYPE_PERSISTENT,
      CACHED_FILE_FROM_SERVER));
  paths_to_delete.push_back(GetCacheFilePath(
      resource_id,
      kWildCard,
      GDataRootDirectory::CACHE_TYPE_TMP,
      CACHED_FILE_FROM_SERVER));

  // For pinned files, filename is "<resource_id>" with no extension, so delete
  // "<resource_id>".
  paths_to_delete.push_back(GetCacheFilePath(
      resource_id,
      std::string(),
      GDataRootDirectory::CACHE_TYPE_PINNED,
      CACHED_FILE_FROM_SERVER));

  for (size_t i = 0; i < paths_to_delete.size(); ++i) {
    DeleteFilesSelectively(paths_to_delete[i],
                           FilePath() /* don't keep any files */);
  }

  // Lock to access cache map.
  base::AutoLock lock(lock_);

  // Now that all file operations have completed, remove from cache map.
  root_->RemoveFromCacheMap(resource_id);

  if (!callback.is_null()) {
    // Invoke callback on calling thread.
    relay_proxy->PostTask(FROM_HERE,
                          base::Bind(callback,
                                     base::PLATFORM_FILE_OK,
                                     resource_id,
                                     std::string()));
  }
}

//=== GDataFileSystem: Cache callbacks for tasks that ran on io thread pool ====

void GDataFileSystem::OnGetCacheState(base::PlatformFileError error,
                                      GDataFile* file,
                                      int cache_state,
                                      const GetCacheStateCallback& callback) {
  DVLOG(1) << "OnGetCacheState: " << error;

  // Lock to let callback access |file| safely.
  base::AutoLock lock(lock_);

  // Invoke callback.
  if (!callback.is_null())
    callback.Run(error, file, cache_state);
}

void GDataFileSystem::OnFilePinned(base::PlatformFileError error,
                                   const std::string& resource_id,
                                   const std::string& md5,
                                   const CacheOperationCallback& callback) {
  if (!callback.is_null())
    callback.Run(error, resource_id, md5);

  NotifyFilePinned(resource_id, md5);
}

void GDataFileSystem::OnFileUnpinned(base::PlatformFileError error,
                                     const std::string& resource_id,
                                     const std::string& md5,
                                     const CacheOperationCallback& callback) {
  if (!callback.is_null())
    callback.Run(error, resource_id, md5);

  NotifyFileUnpinned(resource_id, md5);
}

//============= GDataFileSystem: internal helper functions =====================

void GDataFileSystem::UnsafeInitializeCacheIfNecessary() {
  lock_.AssertAcquired();

  if (cache_initialization_started_) {
    // Cache initialization is either in progress or has completed.
    return;
  }

  // Need to initialize cache.

  cache_initialization_started_ = true;

  PostBlockingPoolSequencedTask(
      kGDataFileSystemToken,
      FROM_HERE,
      base::Bind(&GDataFileSystem::InitializeCacheOnIOThreadPool,
                 base::Unretained(this)));
}

void GDataFileSystem::ScanCacheDirectory(
    GDataRootDirectory::CacheSubDirectoryType sub_dir_type,
    GDataRootDirectory::CacheMap* cache_map) {
  file_util::FileEnumerator enumerator(
      cache_paths_[sub_dir_type],
      false,  // not recursive
      static_cast<file_util::FileEnumerator::FileType>(
          file_util::FileEnumerator::FILES |
          file_util::FileEnumerator::SHOW_SYM_LINKS),
      kWildCard);
  for (FilePath current = enumerator.Next(); !current.empty();
       current = enumerator.Next()) {
    // Extract resource_id and md5 from filename.
    FilePath base_name = current.BaseName();
    std::string resource_id;
    std::string md5;

    // Pinned symlinks have no extension.
    if (sub_dir_type == GDataRootDirectory::CACHE_TYPE_PINNED) {
      resource_id = GDataFileBase::UnescapeUtf8FileName(base_name.value());
    } else {
      FilePath::StringType extension = base_name.Extension();
      if (!extension.empty()) {
        // FilePath::Extension returns ".", so strip it.
        md5 = GDataFileBase::UnescapeUtf8FileName(extension.substr(1));
      }
      resource_id = GDataFileBase::UnescapeUtf8FileName(
          base_name.RemoveExtension().value());
    }

    // Determine cache state.
    int cache_state = GDataFile::CACHE_STATE_NONE;
    // If we're scanning pinned directory and if entry already exists, just
    // update its pinned state.
    if (sub_dir_type == GDataRootDirectory::CACHE_TYPE_PINNED) {
      cache_state |= GDataFile::CACHE_STATE_PINNED;
      GDataRootDirectory::CacheMap::iterator iter =
          cache_map->find(resource_id);
      if (iter != cache_map->end()) {  // Entry exists, update pinned state.
        iter->second->cache_state |= GDataFile::CACHE_STATE_PINNED;
        continue;
      }
      // Entry doesn't exist, this is a special symlink that refers to
      // /dev/null; follow through to create an entry with the PINNED but not
      // PRESENT state.
    } else {
      // Scanning other directories means that cache file is actually present.
      cache_state |= GDataFile::CACHE_STATE_PRESENT;
    }

    // Create and insert new entry into cache map.
    GDataRootDirectory::CacheEntry* entry = new GDataRootDirectory::CacheEntry(
        md5, sub_dir_type, cache_state);
    cache_map->insert(std::make_pair(resource_id, entry));
  }
}

void GDataFileSystem::GetFromCacheInternal(
    const std::string& resource_id,
    const std::string& md5,
    const FilePath& gdata_file_path,
    const GetFromCacheCallback& callback) {
  InitializeCacheIfNecessary();

  PostBlockingPoolSequencedTask(
      kGDataFileSystemToken,
      FROM_HERE,
      base::Bind(&GDataFileSystem::GetFromCacheOnIOThreadPool,
                 base::Unretained(this),
                 resource_id,
                 md5,
                 gdata_file_path,
                 callback,
                 base::MessageLoopProxy::current()));
}

void GDataFileSystem::RunTaskOnIOThreadPool(const base::Closure& task) {
  {
    // Lock to access |in_shutdown_| but release it before running task as
    // task will decide its own locking necessity and timing.
    base::AutoLock lock(lock_);

    // If we're shutting down, just exit.
    if (in_shutdown_)
      return;
  }

  // Reset event to indicate start of IO task.
  on_io_completed_->Reset();

  task.Run();

  // Signal event to indicate completion of IO task.
  on_io_completed_->Signal();
}

bool GDataFileSystem::PostBlockingPoolSequencedTask(
    const std::string& sequence_token_name,
    const tracked_objects::Location& from_here,
    const base::Closure& task) {
  return BrowserThread::PostBlockingPoolSequencedTask(
      sequence_token_name,
      from_here,
      base::Bind(&GDataFileSystem::RunTaskOnIOThreadPool,
                 base::Unretained(this),
                 task));
}

}  // namespace gdata
