// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/sessions2/sessions_sync_manager.h"

#include "chrome/browser/chrome_notification_types.h"
#if !defined(OS_ANDROID)
#include "chrome/browser/network_time/navigation_time_helper.h"
#endif
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/glue/synced_tab_delegate.h"
#include "chrome/browser/sync/glue/synced_window_delegate.h"
#include "chrome/browser/sync/sessions2/sessions_util.h"
#include "chrome/browser/sync/sessions2/synced_window_delegates_getter.h"
#include "chrome/common/url_constants.h"
#include "content/public/browser/favicon_status.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "content/public/common/url_constants.h"
#include "sync/api/sync_error.h"
#include "sync/api/sync_error_factory.h"
#include "sync/api/sync_merge_result.h"
#include "sync/api/time.h"

using content::NavigationEntry;
using sessions::SerializedNavigationEntry;
using syncer::SyncChange;
using syncer::SyncData;

namespace browser_sync {

// Maximum number of favicons to sync.
// TODO(zea): pull this from the server.
static const int kMaxSyncFavicons = 200;

// The maximum number of navigations in each direction we care to sync.
static const int kMaxSyncNavigationCount = 6;

// The URL at which the set of synced tabs is displayed. We treat it differently
// from all other URL's as accessing it triggers a sync refresh of Sessions.
static const char kNTPOpenTabSyncURL[] = "chrome://newtab/#open_tabs";

// Default number of days without activity after which a session is considered
// stale and becomes a candidate for garbage collection.
static const size_t kDefaultStaleSessionThresholdDays = 14;  // 2 weeks.

SessionsSyncManager::SessionsSyncManager(
    Profile* profile,
    SyncInternalApiDelegate* delegate,
    scoped_ptr<LocalSessionEventRouter> router)
    : favicon_cache_(profile, kMaxSyncFavicons),
      local_tab_pool_out_of_sync_(true),
      sync_prefs_(profile->GetPrefs()),
      profile_(profile),
      delegate_(delegate),
      local_session_header_node_id_(TabNodePool2::kInvalidTabNodeID),
      stale_session_threshold_days_(kDefaultStaleSessionThresholdDays),
      local_event_router_(router.Pass()),
      synced_window_getter_(new SyncedWindowDelegatesGetter()) {
}

LocalSessionEventRouter::~LocalSessionEventRouter() {}

SessionsSyncManager::~SessionsSyncManager() {
}

// Returns the GUID-based string that should be used for
// |SessionsSyncManager::current_machine_tag_|.
static std::string BuildMachineTag(const std::string& cache_guid) {
  std::string machine_tag = "session_sync";
  machine_tag.append(cache_guid);
  return machine_tag;
}

syncer::SyncMergeResult SessionsSyncManager::MergeDataAndStartSyncing(
     syncer::ModelType type,
     const syncer::SyncDataList& initial_sync_data,
     scoped_ptr<syncer::SyncChangeProcessor> sync_processor,
     scoped_ptr<syncer::SyncErrorFactory> error_handler) {
  syncer::SyncMergeResult merge_result(type);
  DCHECK(session_tracker_.Empty());
  DCHECK_EQ(0U, local_tab_pool_.Capacity());

  error_handler_ = error_handler.Pass();
  sync_processor_ = sync_processor.Pass();

  local_session_header_node_id_ = TabNodePool2::kInvalidTabNodeID;
  scoped_ptr<DeviceInfo> local_device_info(delegate_->GetLocalDeviceInfo());
  syncer::SyncChangeList new_changes;

  // Make sure we have a machine tag.  We do this now (versus earlier) as it's
  // a conveniently safe time to assert sync is ready and the cache_guid is
  // initialized.
  if (current_machine_tag_.empty())
    InitializeCurrentMachineTag();
  if (local_device_info) {
    current_session_name_ = local_device_info->client_name();
  } else {
    merge_result.set_error(error_handler_->CreateAndUploadError(
        FROM_HERE,
        "Failed to get device info for machine tag."));
    return merge_result;
  }
  session_tracker_.SetLocalSessionTag(current_machine_tag_);

  // First, we iterate over sync data to update our session_tracker_.
  syncer::SyncDataList restored_tabs;
  if (!InitFromSyncModel(initial_sync_data, &restored_tabs, &new_changes)) {
    // The sync db didn't have a header node for us. Create one.
    sync_pb::EntitySpecifics specifics;
    sync_pb::SessionSpecifics* base_specifics = specifics.mutable_session();
    base_specifics->set_session_tag(current_machine_tag());
    sync_pb::SessionHeader* header_s = base_specifics->mutable_header();
    header_s->set_client_name(current_session_name_);
    header_s->set_device_type(DeviceInfo::GetLocalDeviceType());
    syncer::SyncData data = syncer::SyncData::CreateLocalData(
        current_machine_tag(), current_session_name_, specifics);
    new_changes.push_back(syncer::SyncChange(
        FROM_HERE, syncer::SyncChange::ACTION_ADD, data));
  }

#if defined(OS_ANDROID)
  std::string sync_machine_tag(BuildMachineTag(
      delegate_->GetLocalSyncCacheGUID()));
  if (current_machine_tag_.compare(sync_machine_tag) != 0)
    DeleteForeignSessionInternal(sync_machine_tag, &new_changes);
#endif

  // Check if anything has changed on the local client side.
  AssociateWindows(RELOAD_TABS, restored_tabs, &new_changes);
  local_tab_pool_out_of_sync_ = false;

  merge_result.set_error(
      sync_processor_->ProcessSyncChanges(FROM_HERE, new_changes));

  local_event_router_->StartRoutingTo(this);
  return merge_result;
}

void SessionsSyncManager::AssociateWindows(
    ReloadTabsOption option,
    const syncer::SyncDataList& restored_tabs,
    syncer::SyncChangeList* change_output) {
  const std::string local_tag = current_machine_tag();
  sync_pb::SessionSpecifics specifics;
  specifics.set_session_tag(local_tag);
  sync_pb::SessionHeader* header_s = specifics.mutable_header();
  SyncedSession* current_session = session_tracker_.GetSession(local_tag);
  current_session->modified_time = base::Time::Now();
  header_s->set_client_name(current_session_name_);
  header_s->set_device_type(DeviceInfo::GetLocalDeviceType());

  session_tracker_.ResetSessionTracking(local_tag);
  std::set<SyncedWindowDelegate*> windows =
      synced_window_getter_->GetSyncedWindowDelegates();

  for (std::set<SyncedWindowDelegate*>::const_iterator i =
           windows.begin(); i != windows.end(); ++i) {
    // Make sure the window has tabs and a viewable window. The viewable window
    // check is necessary because, for example, when a browser is closed the
    // destructor is not necessarily run immediately. This means its possible
    // for us to get a handle to a browser that is about to be removed. If
    // the tab count is 0 or the window is NULL, the browser is about to be
    // deleted, so we ignore it.
    if (sessions_util::ShouldSyncWindow(*i) &&
        (*i)->GetTabCount() && (*i)->HasWindow()) {
      sync_pb::SessionWindow window_s;
      SessionID::id_type window_id = (*i)->GetSessionId();
      DVLOG(1) << "Associating window " << window_id << " with "
               << (*i)->GetTabCount() << " tabs.";
      window_s.set_window_id(window_id);
      // Note: We don't bother to set selected tab index anymore. We still
      // consume it when receiving foreign sessions, as reading it is free, but
      // it triggers too many sync cycles with too little value to make setting
      // it worthwhile.
      if ((*i)->IsTypeTabbed()) {
        window_s.set_browser_type(
            sync_pb::SessionWindow_BrowserType_TYPE_TABBED);
      } else {
        window_s.set_browser_type(
            sync_pb::SessionWindow_BrowserType_TYPE_POPUP);
      }

      bool found_tabs = false;
      for (int j = 0; j < (*i)->GetTabCount(); ++j) {
        SessionID::id_type tab_id = (*i)->GetTabIdAt(j);
        SyncedTabDelegate* synced_tab = (*i)->GetTabAt(j);

        // GetTabAt can return a null tab; in that case just skip it.
        if (!synced_tab)
          continue;

        if (!synced_tab->HasWebContents()) {
          // For tabs without WebContents update the |tab_id|, as it could have
          // changed after a session restore.
          // Note: We cannot check if a tab is valid if it has no WebContents.
          // We assume any such tab is valid and leave the contents of
          // corresponding sync node unchanged.
          if (synced_tab->GetSyncId() > TabNodePool2::kInvalidTabNodeID &&
              tab_id > TabNodePool2::kInvalidTabID) {
            AssociateRestoredPlaceholderTab(*synced_tab, tab_id,
                                            restored_tabs, change_output);
            found_tabs = true;
            window_s.add_tab(tab_id);
          }
          continue;
        }

        if (RELOAD_TABS == option)
          AssociateTab(synced_tab, change_output);

        // If the tab is valid, it would have been added to the tracker either
        // by the above AssociateTab call (at association time), or by the
        // change processor calling AssociateTab for all modified tabs.
        // Therefore, we can key whether this window has valid tabs based on
        // the tab's presence in the tracker.
        const SessionTab* tab = NULL;
        if (session_tracker_.LookupSessionTab(local_tag, tab_id, &tab)) {
          found_tabs = true;
          window_s.add_tab(tab_id);
        }
      }
      if (found_tabs) {
        sync_pb::SessionWindow* header_window = header_s->add_window();
        *header_window = window_s;

        // Update this window's representation in the synced session tracker.
        session_tracker_.PutWindowInSession(local_tag, window_id);
        BuildSyncedSessionFromSpecifics(local_tag,
                                        window_s,
                                        current_session->modified_time,
                                        current_session->windows[window_id]);
      }
    }
  }
  local_tab_pool_.DeleteUnassociatedTabNodes(change_output);
  session_tracker_.CleanupSession(local_tag);

  // Always update the header.  Sync takes care of dropping this update
  // if the entity specifics are identical (i.e windows, client name did
  // not change).
  sync_pb::EntitySpecifics entity;
  entity.mutable_session()->CopyFrom(specifics);
  syncer::SyncData data = syncer::SyncData::CreateLocalData(
        current_machine_tag(), current_session_name_, entity);
  change_output->push_back(syncer::SyncChange(
        FROM_HERE, syncer::SyncChange::ACTION_UPDATE, data));
}

void SessionsSyncManager::AssociateTab(SyncedTabDelegate* const tab,
                                       syncer::SyncChangeList* change_output) {
  DCHECK(tab->HasWebContents());
  SessionID::id_type tab_id = tab->GetSessionId();
  if (tab->profile() != profile_)
    return;

  if (tab->IsBeingDestroyed()) {
    // This tab is closing.
    TabLinksMap::iterator tab_iter = local_tab_map_.find(tab_id);
    if (tab_iter == local_tab_map_.end()) {
      // We aren't tracking this tab (for example, sync setting page).
      return;
    }
    local_tab_pool_.FreeTabNode(tab_iter->second->tab_node_id(),
                                change_output);
    local_tab_map_.erase(tab_iter);
    return;
  }

  if (!sessions_util::ShouldSyncTab(*tab))
    return;

  TabLinksMap::iterator local_tab_map_iter = local_tab_map_.find(tab_id);
  TabLink* tab_link = NULL;

  if (local_tab_map_iter == local_tab_map_.end()) {
    int tab_node_id = tab->GetSyncId();
    // If there is an old sync node for the tab, reuse it.  If this is a new
    // tab, get a sync node for it.
    if (!local_tab_pool_.IsUnassociatedTabNode(tab_node_id)) {
      tab_node_id = local_tab_pool_.GetFreeTabNode(change_output);
      tab->SetSyncId(tab_node_id);
    }
    local_tab_pool_.AssociateTabNode(tab_node_id, tab_id);
    tab_link = new TabLink(tab_node_id, tab);
    local_tab_map_[tab_id] = make_linked_ptr<TabLink>(tab_link);
  } else {
    // This tab is already associated with a sync node, reuse it.
    // Note: on some platforms the tab object may have changed, so we ensure
    // the tab link is up to date.
    tab_link = local_tab_map_iter->second.get();
    local_tab_map_iter->second->set_tab(tab);
  }
  DCHECK(tab_link);
  DCHECK_NE(tab_link->tab_node_id(), TabNodePool2::kInvalidTabNodeID);
  DVLOG(1) << "Reloading tab " << tab_id << " from window "
           << tab->GetWindowId();

  // Write to sync model.
  sync_pb::EntitySpecifics specifics;
  LocalTabDelegateToSpecifics(*tab, specifics.mutable_session());
  syncer::SyncData data = syncer::SyncData::CreateLocalData(
      TabNodePool2::TabIdToTag(current_machine_tag_,
                               tab_link->tab_node_id()),
      current_session_name_,
      specifics);
  change_output->push_back(syncer::SyncChange(
      FROM_HERE, syncer::SyncChange::ACTION_UPDATE, data));

  const GURL new_url = GetCurrentVirtualURL(*tab);
  if (new_url != tab_link->url()) {
    tab_link->set_url(new_url);
    favicon_cache_.OnFaviconVisited(new_url, GetCurrentFaviconURL(*tab));
  }

  session_tracker_.GetSession(current_machine_tag())->modified_time =
      base::Time::Now();
}

void SessionsSyncManager::RebuildAssociations() {
  syncer::SyncDataList data(
      sync_processor_->GetAllSyncData(syncer::SESSIONS));
  scoped_ptr<syncer::SyncErrorFactory> error_handler(error_handler_.Pass());
  scoped_ptr<syncer::SyncChangeProcessor> processor(sync_processor_.Pass());

  StopSyncing(syncer::SESSIONS);
  MergeDataAndStartSyncing(
      syncer::SESSIONS, data, processor.Pass(), error_handler.Pass());
}

void SessionsSyncManager::OnLocalTabModified(SyncedTabDelegate* modified_tab) {
  const content::NavigationEntry* entry = modified_tab->GetActiveEntry();
  if (!modified_tab->IsBeingDestroyed() &&
      entry &&
      entry->GetVirtualURL().is_valid() &&
      entry->GetVirtualURL().spec() == kNTPOpenTabSyncURL) {
    DVLOG(1) << "Triggering sync refresh for sessions datatype.";
    const syncer::ModelTypeSet types(syncer::SESSIONS);
    content::NotificationService::current()->Notify(
        chrome::NOTIFICATION_SYNC_REFRESH_LOCAL,
        content::Source<Profile>(profile_),
        content::Details<const syncer::ModelTypeSet>(&types));
  }

  if (local_tab_pool_out_of_sync_) {
    // If our tab pool is corrupt, pay the price of a full re-association to
    // fix things up.  This takes care of the new tab modification as well.
    RebuildAssociations();
    DCHECK(!local_tab_pool_out_of_sync_);
    return;
  }

  syncer::SyncChangeList changes;
  // Associate tabs first so the synced session tracker is aware of them.
  AssociateTab(modified_tab, &changes);
  // Note, we always associate windows because it's possible a tab became
  // "interesting" by going to a valid URL, in which case it needs to be added
  // to the window's tab information.
  AssociateWindows(DONT_RELOAD_TABS, syncer::SyncDataList(), &changes);
  sync_processor_->ProcessSyncChanges(FROM_HERE, changes);
}

void SessionsSyncManager::OnFaviconPageUrlsUpdated(
    const std::set<GURL>& updated_favicon_page_urls) {
  // TODO(zea): consider a separate container for tabs with outstanding favicon
  // loads so we don't have to iterate through all tabs comparing urls.
  for (std::set<GURL>::const_iterator i = updated_favicon_page_urls.begin();
       i != updated_favicon_page_urls.end(); ++i) {
    for (TabLinksMap::iterator tab_iter = local_tab_map_.begin();
         tab_iter != local_tab_map_.end();
         ++tab_iter) {
      if (tab_iter->second->url() == *i)
        favicon_cache_.OnPageFaviconUpdated(*i);
    }
  }
}

void SessionsSyncManager::StopSyncing(syncer::ModelType type) {
  local_event_router_->Stop();
  sync_processor_.reset(NULL);
  error_handler_.reset();
  session_tracker_.Clear();
  local_tab_map_.clear();
  local_tab_pool_.Clear();
  current_machine_tag_.clear();
  current_session_name_.clear();
  local_session_header_node_id_ = TabNodePool2::kInvalidTabNodeID;
}

syncer::SyncDataList SessionsSyncManager::GetAllSyncData(
    syncer::ModelType type) const {
  syncer::SyncDataList list;
  const SyncedSession* session = NULL;
  if (!session_tracker_.LookupLocalSession(&session))
    return syncer::SyncDataList();

  // First construct the header node.
  sync_pb::EntitySpecifics header_entity;
  header_entity.mutable_session()->set_session_tag(current_machine_tag());
  sync_pb::SessionHeader* header_specifics =
      header_entity.mutable_session()->mutable_header();
  header_specifics->MergeFrom(session->ToSessionHeader());
  syncer::SyncData data = syncer::SyncData::CreateLocalData(
        current_machine_tag(), current_session_name_, header_entity);
  list.push_back(data);

  SyncedSession::SyncedWindowMap::const_iterator win_iter;
  for (win_iter = session->windows.begin();
       win_iter != session->windows.end(); ++win_iter) {
    std::vector<SessionTab*>::const_iterator tabs_iter;
    for (tabs_iter = win_iter->second->tabs.begin();
         tabs_iter != win_iter->second->tabs.end(); ++tabs_iter) {
      sync_pb::EntitySpecifics entity;
      sync_pb::SessionSpecifics* specifics = entity.mutable_session();
      specifics->mutable_tab()->MergeFrom((*tabs_iter)->ToSyncData());
      specifics->set_session_tag(current_machine_tag_);

      TabLinksMap::const_iterator tab_map_iter = local_tab_map_.find(
          (*tabs_iter)->tab_id.id());
      DCHECK(tab_map_iter != local_tab_map_.end());
      specifics->set_tab_node_id(tab_map_iter->second->tab_node_id());
      syncer::SyncData data = syncer::SyncData::CreateLocalData(
          TabNodePool2::TabIdToTag(current_machine_tag_,
                                   specifics->tab_node_id()),
          current_session_name_,
          entity);
      list.push_back(data);
    }
  }
  return list;
}

bool SessionsSyncManager::GetLocalSession(
    const SyncedSession* * local_session) {
  if (current_machine_tag_.empty())
    return false;
  *local_session = session_tracker_.GetSession(current_machine_tag());
  return true;
}

syncer::SyncError SessionsSyncManager::ProcessSyncChanges(
    const tracked_objects::Location& from_here,
    const syncer::SyncChangeList& change_list) {
  if (!sync_processor_.get()) {
    syncer::SyncError error(FROM_HERE,
                            syncer::SyncError::DATATYPE_ERROR,
                            "Models not yet associated.",
                            syncer::SESSIONS);
    return error;
  }

  for (syncer::SyncChangeList::const_iterator it = change_list.begin();
       it != change_list.end(); ++it) {
    DCHECK(it->IsValid());
    DCHECK(it->sync_data().GetSpecifics().has_session());
    const sync_pb::SessionSpecifics& session =
        it->sync_data().GetSpecifics().session();
    switch (it->change_type()) {
      case syncer::SyncChange::ACTION_DELETE:
        // Deletions are all or nothing (since we only ever delete entire
        // sessions). Therefore we don't care if it's a tab node or meta node,
        // and just ensure we've disassociated.
        if (current_machine_tag() == session.session_tag()) {
          // Another client has attempted to delete our local data (possibly by
          // error or a clock is inaccurate). Just ignore the deletion for now
          // to avoid any possible ping-pong delete/reassociate sequence, but
          // remember that this happened as our TabNodePool is inconsistent.
          local_tab_pool_out_of_sync_ = true;
          LOG(WARNING) << "Local session data deleted. Ignoring until next "
                       << "local navigation event.";
        } else if (session.has_header()) {
          // Disassociate only when header node is deleted. For tab node
          // deletions, the header node will be updated and foreign tab will
          // get deleted.
          DisassociateForeignSession(session.session_tag());
        }
        continue;
      case syncer::SyncChange::ACTION_ADD:
      case syncer::SyncChange::ACTION_UPDATE:
        if (current_machine_tag() == session.session_tag()) {
          // We should only ever receive a change to our own machine's session
          // info if encryption was turned on. In that case, the data is still
          // the same, so we can ignore.
          LOG(WARNING) << "Dropping modification to local session.";
          return syncer::SyncError();
        }
        UpdateTrackerWithForeignSession(
            session, it->sync_data().GetRemoteModifiedTime());
        break;
      default:
        NOTREACHED() << "Processing sync changes failed, unknown change type.";
    }
  }

  content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_FOREIGN_SESSION_UPDATED,
      content::Source<Profile>(profile_),
      content::NotificationService::NoDetails());
  return syncer::SyncError();
}

syncer::SyncChange SessionsSyncManager::TombstoneTab(
    const sync_pb::SessionSpecifics& tab) {
  if (!tab.has_tab_node_id()) {
    LOG(WARNING) << "Old sessions node without tab node id; can't tombstone.";
    return syncer::SyncChange();
  } else {
    return syncer::SyncChange(
        FROM_HERE,
        SyncChange::ACTION_DELETE,
        SyncData::CreateLocalDelete(
            TabNodePool2::TabIdToTag(current_machine_tag(),
                                     tab.tab_node_id()),
            syncer::SESSIONS));
  }
}

bool SessionsSyncManager::GetAllForeignSessions(
    std::vector<const SyncedSession*>* sessions) {
  return session_tracker_.LookupAllForeignSessions(sessions);
}

bool SessionsSyncManager::InitFromSyncModel(
    const syncer::SyncDataList& sync_data,
    syncer::SyncDataList* restored_tabs,
    syncer::SyncChangeList* new_changes) {
  bool found_current_header = false;
  for (syncer::SyncDataList::const_iterator it = sync_data.begin();
       it != sync_data.end();
       ++it) {
    const syncer::SyncData& data = *it;
    DCHECK(data.GetSpecifics().has_session());
    const sync_pb::SessionSpecifics& specifics = data.GetSpecifics().session();
    if (specifics.session_tag().empty() ||
           (specifics.has_tab() && (!specifics.has_tab_node_id() ||
                                    !specifics.tab().has_tab_id()))) {
      syncer::SyncChange tombstone(TombstoneTab(specifics));
      if (tombstone.IsValid())
        new_changes->push_back(tombstone);
    } else if (specifics.session_tag() != current_machine_tag()) {
      UpdateTrackerWithForeignSession(specifics, data.GetRemoteModifiedTime());
    } else {
      // This is previously stored local session information.
      if (specifics.has_header() && !found_current_header) {
        // This is our previous header node, reuse it.
        found_current_header = true;
        if (specifics.header().has_client_name())
          current_session_name_ = specifics.header().client_name();
      } else {
        if (specifics.has_header() || !specifics.has_tab()) {
          LOG(WARNING) << "Found more than one session header node with local "
                       << "tag.";
          syncer::SyncChange tombstone(TombstoneTab(specifics));
          if (tombstone.IsValid())
            new_changes->push_back(tombstone);
        } else {
          // This is a valid old tab node, add it to the pool so it can be
          // reused for reassociation.
          local_tab_pool_.AddTabNode(specifics.tab_node_id());
          restored_tabs->push_back(*it);
        }
      }
    }
  }
  return found_current_header;
}

void SessionsSyncManager::UpdateTrackerWithForeignSession(
    const sync_pb::SessionSpecifics& specifics,
    const base::Time& modification_time) {
  std::string foreign_session_tag = specifics.session_tag();
  DCHECK_NE(foreign_session_tag, current_machine_tag());

  SyncedSession* foreign_session =
      session_tracker_.GetSession(foreign_session_tag);
  if (specifics.has_header()) {
    // Read in the header data for this foreign session.
    // Header data contains window information and ordered tab id's for each
    // window.

    // Load (or create) the SyncedSession object for this client.
    const sync_pb::SessionHeader& header = specifics.header();
    PopulateSessionHeaderFromSpecifics(header,
                                       modification_time,
                                       foreign_session);

    // Reset the tab/window tracking for this session (must do this before
    // we start calling PutWindowInSession and PutTabInWindow so that all
    // unused tabs/windows get cleared by the CleanupSession(...) call).
    session_tracker_.ResetSessionTracking(foreign_session_tag);

    // Process all the windows and their tab information.
    int num_windows = header.window_size();
    DVLOG(1) << "Associating " << foreign_session_tag << " with "
             << num_windows << " windows.";

    for (int i = 0; i < num_windows; ++i) {
      const sync_pb::SessionWindow& window_s = header.window(i);
      SessionID::id_type window_id = window_s.window_id();
      session_tracker_.PutWindowInSession(foreign_session_tag,
                                          window_id);
      BuildSyncedSessionFromSpecifics(foreign_session_tag,
                                      window_s,
                                      modification_time,
                                      foreign_session->windows[window_id]);
    }
    // Delete any closed windows and unused tabs as necessary.
    session_tracker_.CleanupSession(foreign_session_tag);
  } else if (specifics.has_tab()) {
    const sync_pb::SessionTab& tab_s = specifics.tab();
    SessionID::id_type tab_id = tab_s.tab_id();
    SessionTab* tab =
        session_tracker_.GetTab(foreign_session_tag,
                                tab_id,
                                specifics.tab_node_id());

    // Update SessionTab based on protobuf.
    tab->SetFromSyncData(tab_s, modification_time);

    // If a favicon or favicon urls are present, load the URLs and visit
    // times into the in-memory favicon cache.
    RefreshFaviconVisitTimesFromForeignTab(tab_s, modification_time);

    // Update the last modified time.
    if (foreign_session->modified_time < modification_time)
      foreign_session->modified_time = modification_time;
  } else {
    LOG(WARNING) << "Ignoring foreign session node with missing header/tab "
                 << "fields and tag " << foreign_session_tag << ".";
  }
}

void SessionsSyncManager::InitializeCurrentMachineTag() {
  DCHECK(current_machine_tag_.empty());
  std::string persisted_guid;
  persisted_guid = sync_prefs_.GetSyncSessionsGUID();
  if (!persisted_guid.empty()) {
    current_machine_tag_ = persisted_guid;
    DVLOG(1) << "Restoring persisted session sync guid: " << persisted_guid;
  } else {
    current_machine_tag_ = BuildMachineTag(delegate_->GetLocalSyncCacheGUID());
    DVLOG(1) << "Creating session sync guid: " << current_machine_tag_;
    sync_prefs_.SetSyncSessionsGUID(current_machine_tag_);
  }

  local_tab_pool_.SetMachineTag(current_machine_tag_);
}

// static
void SessionsSyncManager::PopulateSessionHeaderFromSpecifics(
    const sync_pb::SessionHeader& header_specifics,
    base::Time mtime,
    SyncedSession* session_header) {
  if (header_specifics.has_client_name())
    session_header->session_name = header_specifics.client_name();
  if (header_specifics.has_device_type()) {
    switch (header_specifics.device_type()) {
      case sync_pb::SyncEnums_DeviceType_TYPE_WIN:
        session_header->device_type = SyncedSession::TYPE_WIN;
        break;
      case sync_pb::SyncEnums_DeviceType_TYPE_MAC:
        session_header->device_type = SyncedSession::TYPE_MACOSX;
        break;
      case sync_pb::SyncEnums_DeviceType_TYPE_LINUX:
        session_header->device_type = SyncedSession::TYPE_LINUX;
        break;
      case sync_pb::SyncEnums_DeviceType_TYPE_CROS:
        session_header->device_type = SyncedSession::TYPE_CHROMEOS;
        break;
      case sync_pb::SyncEnums_DeviceType_TYPE_PHONE:
        session_header->device_type = SyncedSession::TYPE_PHONE;
        break;
      case sync_pb::SyncEnums_DeviceType_TYPE_TABLET:
        session_header->device_type = SyncedSession::TYPE_TABLET;
        break;
      case sync_pb::SyncEnums_DeviceType_TYPE_OTHER:
        // Intentionally fall-through
      default:
        session_header->device_type = SyncedSession::TYPE_OTHER;
        break;
    }
  }
  session_header->modified_time = mtime;
}

// static
void SessionsSyncManager::BuildSyncedSessionFromSpecifics(
    const std::string& session_tag,
    const sync_pb::SessionWindow& specifics,
    base::Time mtime,
    SessionWindow* session_window) {
  if (specifics.has_window_id())
    session_window->window_id.set_id(specifics.window_id());
  if (specifics.has_selected_tab_index())
    session_window->selected_tab_index = specifics.selected_tab_index();
  if (specifics.has_browser_type()) {
    if (specifics.browser_type() ==
        sync_pb::SessionWindow_BrowserType_TYPE_TABBED) {
      session_window->type = 1;
    } else {
      session_window->type = 2;
    }
  }
  session_window->timestamp = mtime;
  session_window->tabs.resize(specifics.tab_size(), NULL);
  for (int i = 0; i < specifics.tab_size(); i++) {
    SessionID::id_type tab_id = specifics.tab(i);
    session_tracker_.PutTabInWindow(session_tag,
                                    session_window->window_id.id(),
                                    tab_id,
                                    i);
  }
}

void SessionsSyncManager::RefreshFaviconVisitTimesFromForeignTab(
    const sync_pb::SessionTab& tab, const base::Time& modification_time) {
  // First go through and iterate over all the navigations, checking if any
  // have valid favicon urls.
  for (int i = 0; i < tab.navigation_size(); ++i) {
    if (!tab.navigation(i).favicon_url().empty()) {
      const std::string& page_url = tab.navigation(i).virtual_url();
      const std::string& favicon_url = tab.navigation(i).favicon_url();
      favicon_cache_.OnReceivedSyncFavicon(GURL(page_url),
                                           GURL(favicon_url),
                                           std::string(),
                                           syncer::TimeToProtoTime(
                                               modification_time));
    }
  }
}

bool SessionsSyncManager::GetSyncedFaviconForPageURL(
    const std::string& page_url,
    scoped_refptr<base::RefCountedMemory>* favicon_png) const {
  return favicon_cache_.GetSyncedFaviconForPageURL(GURL(page_url), favicon_png);
}

void SessionsSyncManager::DeleteForeignSession(const std::string& tag) {
  syncer::SyncChangeList changes;
  DeleteForeignSessionInternal(tag, &changes);
  sync_processor_->ProcessSyncChanges(FROM_HERE, changes);
}

void SessionsSyncManager::DeleteForeignSessionInternal(
    const std::string& tag, syncer::SyncChangeList* change_output) {
 if (tag == current_machine_tag()) {
    LOG(ERROR) << "Attempting to delete local session. This is not currently "
               << "supported.";
    return;
  }

  std::set<int> tab_node_ids_to_delete;
  session_tracker_.LookupTabNodeIds(tag, &tab_node_ids_to_delete);
  if (!DisassociateForeignSession(tag)) {
    // We don't have any data for this session, our work here is done!
    return;
  }

  // Prepare deletes for the meta-node as well as individual tab nodes.
  change_output->push_back(syncer::SyncChange(
      FROM_HERE,
      SyncChange::ACTION_DELETE,
      SyncData::CreateLocalDelete(tag, syncer::SESSIONS)));

  for (std::set<int>::const_iterator it = tab_node_ids_to_delete.begin();
       it != tab_node_ids_to_delete.end();
       ++it) {
    change_output->push_back(syncer::SyncChange(
        FROM_HERE,
        SyncChange::ACTION_DELETE,
        SyncData::CreateLocalDelete(TabNodePool2::TabIdToTag(tag, *it),
                                    syncer::SESSIONS)));
  }
  content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_FOREIGN_SESSION_UPDATED,
      content::Source<Profile>(profile_),
      content::NotificationService::NoDetails());
}

bool SessionsSyncManager::DisassociateForeignSession(
    const std::string& foreign_session_tag) {
  if (foreign_session_tag == current_machine_tag()) {
    DVLOG(1) << "Local session deleted! Doing nothing until a navigation is "
             << "triggered.";
    return false;
  }
  DVLOG(1) << "Disassociating session " << foreign_session_tag;
  return session_tracker_.DeleteSession(foreign_session_tag);
}

// static
GURL SessionsSyncManager::GetCurrentVirtualURL(
    const SyncedTabDelegate& tab_delegate) {
  const int current_index = tab_delegate.GetCurrentEntryIndex();
  const int pending_index = tab_delegate.GetPendingEntryIndex();
  const NavigationEntry* current_entry =
      (current_index == pending_index) ?
      tab_delegate.GetPendingEntry() :
      tab_delegate.GetEntryAtIndex(current_index);
  return current_entry->GetVirtualURL();
}

// static
GURL SessionsSyncManager::GetCurrentFaviconURL(
    const SyncedTabDelegate& tab_delegate) {
  const int current_index = tab_delegate.GetCurrentEntryIndex();
  const int pending_index = tab_delegate.GetPendingEntryIndex();
  const NavigationEntry* current_entry =
      (current_index == pending_index) ?
      tab_delegate.GetPendingEntry() :
      tab_delegate.GetEntryAtIndex(current_index);
  return (current_entry->GetFavicon().valid ?
          current_entry->GetFavicon().url :
          GURL());
}

bool SessionsSyncManager::GetForeignSession(
    const std::string& tag,
    std::vector<const SessionWindow*>* windows) {
  return session_tracker_.LookupSessionWindows(tag, windows);
}

bool SessionsSyncManager::GetForeignTab(
    const std::string& tag,
    const SessionID::id_type tab_id,
    const SessionTab** tab) {
  const SessionTab* synced_tab = NULL;
  bool success = session_tracker_.LookupSessionTab(tag,
                                                   tab_id,
                                                   &synced_tab);
  if (success)
    *tab = synced_tab;
  return success;
}

void SessionsSyncManager::LocalTabDelegateToSpecifics(
    const SyncedTabDelegate& tab_delegate,
    sync_pb::SessionSpecifics* specifics) {
  SessionTab* session_tab = NULL;
  session_tab =
      session_tracker_.GetTab(current_machine_tag(),
                              tab_delegate.GetSessionId(),
                              tab_delegate.GetSyncId());
  SetSessionTabFromDelegate(tab_delegate, base::Time::Now(), session_tab);
  sync_pb::SessionTab tab_s = session_tab->ToSyncData();
  specifics->set_session_tag(current_machine_tag_);
  specifics->set_tab_node_id(tab_delegate.GetSyncId());
  specifics->mutable_tab()->CopyFrom(tab_s);
}

void SessionsSyncManager::AssociateRestoredPlaceholderTab(
    const SyncedTabDelegate& tab_delegate,
    SessionID::id_type new_tab_id,
    const syncer::SyncDataList& restored_tabs,
    syncer::SyncChangeList* change_output) {
  DCHECK_NE(tab_delegate.GetSyncId(), TabNodePool2::kInvalidTabNodeID);
  // Rewrite the tab using |restored_tabs| to retrieve the specifics.
  if (restored_tabs.empty()) {
    DLOG(WARNING) << "Can't Update tab ID.";
    return;
  }

  for (syncer::SyncDataList::const_iterator it = restored_tabs.begin();
       it != restored_tabs.end();
       ++it) {
    if (it->GetSpecifics().session().tab_node_id() !=
        tab_delegate.GetSyncId()) {
      continue;
    }

    sync_pb::EntitySpecifics entity;
    sync_pb::SessionSpecifics* specifics = entity.mutable_session();
    specifics->CopyFrom(it->GetSpecifics().session());
    DCHECK(specifics->has_tab());

    // Update tab node pool with the new association.
    local_tab_pool_.ReassociateTabNode(tab_delegate.GetSyncId(),
                                       new_tab_id);
    TabLink* tab_link = new TabLink(tab_delegate.GetSyncId(),
                                    &tab_delegate);
    local_tab_map_[new_tab_id] = make_linked_ptr<TabLink>(tab_link);

    if (specifics->tab().tab_id() == new_tab_id)
      return;

    // The tab_id changed (e.g due to session restore), so update sync.
    specifics->mutable_tab()->set_tab_id(new_tab_id);
    syncer::SyncData data = syncer::SyncData::CreateLocalData(
        TabNodePool2::TabIdToTag(current_machine_tag_,
                                 specifics->tab_node_id()),
        current_session_name_,
        entity);
    change_output->push_back(syncer::SyncChange(
        FROM_HERE, syncer::SyncChange::ACTION_UPDATE, data));
    return;
  }
}

// static.
void SessionsSyncManager::SetSessionTabFromDelegate(
      const SyncedTabDelegate& tab_delegate,
      base::Time mtime,
      SessionTab* session_tab) {
  DCHECK(session_tab);
  session_tab->window_id.set_id(tab_delegate.GetWindowId());
  session_tab->tab_id.set_id(tab_delegate.GetSessionId());
  session_tab->tab_visual_index = 0;
  session_tab->current_navigation_index = tab_delegate.GetCurrentEntryIndex();
  session_tab->pinned = tab_delegate.IsPinned();
  session_tab->extension_app_id = tab_delegate.GetExtensionAppId();
  session_tab->user_agent_override.clear();
  session_tab->timestamp = mtime;
  const int current_index = tab_delegate.GetCurrentEntryIndex();
  const int pending_index = tab_delegate.GetPendingEntryIndex();
  const int min_index = std::max(0, current_index - kMaxSyncNavigationCount);
  const int max_index = std::min(current_index + kMaxSyncNavigationCount,
                                 tab_delegate.GetEntryCount());
  bool is_managed = tab_delegate.ProfileIsManaged();
  session_tab->navigations.clear();

#if !defined(OS_ANDROID)
  // For getting navigation time in network time.
  NavigationTimeHelper* nav_time_helper =
      tab_delegate.HasWebContents() ?
          NavigationTimeHelper::FromWebContents(tab_delegate.GetWebContents()) :
          NULL;
#endif

  for (int i = min_index; i < max_index; ++i) {
    const NavigationEntry* entry = (i == pending_index) ?
        tab_delegate.GetPendingEntry() : tab_delegate.GetEntryAtIndex(i);
    DCHECK(entry);
    if (!entry->GetVirtualURL().is_valid())
      continue;

    scoped_ptr<content::NavigationEntry> network_time_entry(
        content::NavigationEntry::Create(*entry));
#if !defined(OS_ANDROID)
    if (nav_time_helper) {
      network_time_entry->SetTimestamp(
          nav_time_helper->GetNavigationTime(entry));
    }
#endif

    session_tab->navigations.push_back(
        SerializedNavigationEntry::FromNavigationEntry(i, *network_time_entry));
    if (is_managed) {
      session_tab->navigations.back().set_blocked_state(
          SerializedNavigationEntry::STATE_ALLOWED);
    }
  }

  if (is_managed) {
    const std::vector<const NavigationEntry*>& blocked_navigations =
        *tab_delegate.GetBlockedNavigations();
    int offset = session_tab->navigations.size();
    for (size_t i = 0; i < blocked_navigations.size(); ++i) {
      session_tab->navigations.push_back(
          SerializedNavigationEntry::FromNavigationEntry(
              i + offset, *blocked_navigations[i]));
      session_tab->navigations.back().set_blocked_state(
          SerializedNavigationEntry::STATE_BLOCKED);
      // TODO(bauerb): Add categories
    }
  }
  session_tab->session_storage_persistent_id.clear();
}

FaviconCache* SessionsSyncManager::GetFaviconCache() {
  return &favicon_cache_;
}

void SessionsSyncManager::DoGarbageCollection() {
  std::vector<const SyncedSession*> sessions;
  if (!GetAllForeignSessions(&sessions))
    return;  // No foreign sessions.

  // Iterate through all the sessions and delete any with age older than
  // |stale_session_threshold_days_|.
  syncer::SyncChangeList changes;
  for (std::vector<const SyncedSession*>::const_iterator iter =
           sessions.begin(); iter != sessions.end(); ++iter) {
    const SyncedSession* session = *iter;
    int session_age_in_days =
        (base::Time::Now() - session->modified_time).InDays();
    std::string session_tag = session->session_tag;
    if (session_age_in_days > 0 &&  // If false, local clock is not trustworty.
        static_cast<size_t>(session_age_in_days) >
            stale_session_threshold_days_) {
      DVLOG(1) << "Found stale session " << session_tag
               << " with age " << session_age_in_days << ", deleting.";
      DeleteForeignSessionInternal(session_tag, &changes);
    }
  }

  if (!changes.empty())
    sync_processor_->ProcessSyncChanges(FROM_HERE, changes);
}

};  // namespace browser_sync
