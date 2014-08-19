// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_USERS_AVATAR_USER_IMAGE_SYNC_OBSERVER_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_USERS_AVATAR_USER_IMAGE_SYNC_OBSERVER_H_

#include <string>

#include "base/memory/scoped_ptr.h"
#include "base/observer_list.h"
#include "chrome/browser/prefs/pref_service_syncable_observer.h"
#include "content/public/browser/notification_observer.h"

class PrefChangeRegistrar;
class PrefServiceSyncable;
class Profile;

namespace content {
class NotificationRegistrar;
}
namespace user_prefs {
class PrefRegistrySyncable;
}

namespace user_manager {
class User;
}

namespace chromeos {

// This class is responsible for keeping local user image synced with
// image saved in syncable preference.
class UserImageSyncObserver: public PrefServiceSyncableObserver,
                             public content::NotificationObserver {
 public:
  class Observer {
   public:
    // Called right after image info synced (i.e. |is_synced| became |true|).
    // |local_image_updated| indicates if we desided to update local image in
    // result of sync.
    virtual void OnInitialSync(bool local_image_updated) = 0;
    virtual ~Observer();
  };

 public:
  explicit UserImageSyncObserver(const user_manager::User* user);
  virtual ~UserImageSyncObserver();

  // Register syncable preference for profile.
  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  // Returns |true| if sync was initialized and prefs have actual state.
  bool is_synced() const { return is_synced_; }

  // Adds |observer| into observers list.
  void AddObserver(Observer* observer);
  // Removes |observer| from observers list.
  void RemoveObserver(Observer* observer);

 private:
  // PrefServiceSyncableObserver implementation.
  virtual void OnIsSyncingChanged() OVERRIDE;

  // content::NotificationObserver implementation.
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE;

  // Called after user profile was loaded.
  void OnProfileGained(Profile* profile);

  // Called when sync servise started it's work and we are able to sync needed
  // preferences.
  void OnInitialSync();

  // Called when preference |pref_name| was changed.j
  void OnPreferenceChanged(const std::string& pref_name);

  // Saves local image preferences to sync.
  void UpdateSyncedImageFromLocal();

  // Saves sync preferences to local state and updates user image.
  void UpdateLocalImageFromSynced();

  // Gets synced image index. Returns false if user has no needed preferences.
  bool GetSyncedImageIndex(int* result);

  // If it is allowed to change user image now.
  bool CanUpdateLocalImageNow();

  const user_manager::User* user_;
  scoped_ptr<PrefChangeRegistrar> pref_change_registrar_;
  scoped_ptr<content::NotificationRegistrar> notification_registrar_;
  PrefServiceSyncable* prefs_;
  bool is_synced_;
  // Indicates if local user image changed during initialization.
  bool local_image_changed_;
  ObserverList<Observer> observer_list_;
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_USERS_AVATAR_USER_IMAGE_SYNC_OBSERVER_H_

