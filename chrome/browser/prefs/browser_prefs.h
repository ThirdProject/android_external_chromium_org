// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PREFS_BROWSER_PREFS_H_
#define CHROME_BROWSER_PREFS_BROWSER_PREFS_H_

class PrefService;
class PrefServiceSimple;
class PrefServiceSyncable;
class Profile;

namespace chrome {

// Makes the PrefService objects aware of all the prefs.
void RegisterLocalState(PrefServiceSimple* local_state);
void RegisterUserPrefs(PrefServiceSyncable* user_prefs);

// Migrates prefs from |local_state| to |profile|'s pref store.
void MigrateBrowserPrefs(Profile* profile, PrefServiceSimple* local_state);

// Migrates prefs in |profile|'s pref store.
void MigrateUserPrefs(Profile* profile);

}  // namespace chrome

#endif  // CHROME_BROWSER_PREFS_BROWSER_PREFS_H_
