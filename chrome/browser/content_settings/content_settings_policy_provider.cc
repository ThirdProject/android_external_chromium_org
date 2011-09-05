// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/content_settings/content_settings_policy_provider.h"

#include <string>
#include <vector>

#include "base/command_line.h"
#include "chrome/browser/content_settings/content_settings_pattern.h"
#include "chrome/browser/content_settings/content_settings_utils.h"
#include "chrome/browser/prefs/pref_service.h"
#include "chrome/browser/prefs/scoped_user_pref_update.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_notification_types.h"
#include "chrome/common/pref_names.h"
#include "content/browser/browser_thread.h"
#include "content/common/notification_service.h"
#include "content/common/notification_source.h"
#include "webkit/plugins/npapi/plugin_group.h"
#include "webkit/plugins/npapi/plugin_list.h"

namespace {

// The preferences used to manage ContentSettingsTypes.
const char* kPrefToManageType[CONTENT_SETTINGS_NUM_TYPES] = {
  prefs::kManagedDefaultCookiesSetting,
  prefs::kManagedDefaultImagesSetting,
  prefs::kManagedDefaultJavaScriptSetting,
  prefs::kManagedDefaultPluginsSetting,
  prefs::kManagedDefaultPopupsSetting,
  prefs::kManagedDefaultGeolocationSetting,
  prefs::kManagedDefaultNotificationsSetting,
  NULL,
  prefs::kManagedDefaultAutoSelectCertificateSetting,
};

struct PrefsForManagedContentSettingsMapEntry {
  const char* pref_name;
  ContentSettingsType content_type;
  ContentSetting setting;
};

const PrefsForManagedContentSettingsMapEntry
    kPrefsForManagedContentSettingsMap[] = {
  {
    prefs::kManagedAutoSelectCertificateForUrls,
    CONTENT_SETTINGS_TYPE_AUTO_SELECT_CERTIFICATE,
    CONTENT_SETTING_ALLOW
  }, {
    prefs::kManagedCookiesAllowedForUrls,
    CONTENT_SETTINGS_TYPE_COOKIES,
    CONTENT_SETTING_ALLOW
  }, {
    prefs::kManagedCookiesSessionOnlyForUrls,
    CONTENT_SETTINGS_TYPE_COOKIES,
    CONTENT_SETTING_SESSION_ONLY
  }, {
    prefs::kManagedCookiesBlockedForUrls,
    CONTENT_SETTINGS_TYPE_COOKIES,
    CONTENT_SETTING_BLOCK
  }, {
    prefs::kManagedImagesAllowedForUrls,
    CONTENT_SETTINGS_TYPE_IMAGES,
    CONTENT_SETTING_ALLOW
  }, {
    prefs::kManagedImagesBlockedForUrls,
    CONTENT_SETTINGS_TYPE_IMAGES,
    CONTENT_SETTING_BLOCK
  }, {
    prefs::kManagedJavaScriptAllowedForUrls,
    CONTENT_SETTINGS_TYPE_JAVASCRIPT,
    CONTENT_SETTING_ALLOW
  }, {
    prefs::kManagedJavaScriptBlockedForUrls,
    CONTENT_SETTINGS_TYPE_JAVASCRIPT,
    CONTENT_SETTING_BLOCK
  }, {
    prefs::kManagedPluginsAllowedForUrls,
    CONTENT_SETTINGS_TYPE_PLUGINS,
    CONTENT_SETTING_ALLOW
  }, {
    prefs::kManagedPluginsBlockedForUrls,
    CONTENT_SETTINGS_TYPE_PLUGINS,
    CONTENT_SETTING_BLOCK
  }, {
    prefs::kManagedPopupsAllowedForUrls,
    CONTENT_SETTINGS_TYPE_POPUPS,
    CONTENT_SETTING_ALLOW
  }, {
    prefs::kManagedPopupsBlockedForUrls,
    CONTENT_SETTINGS_TYPE_POPUPS,
    CONTENT_SETTING_BLOCK
  }
};

}  // namespace

namespace content_settings {

PolicyDefaultProvider::PolicyDefaultProvider(PrefService* prefs)
    : prefs_(prefs) {
  // Read global defaults.
  DCHECK_EQ(arraysize(kPrefToManageType),
            static_cast<size_t>(CONTENT_SETTINGS_NUM_TYPES));
  ReadManagedDefaultSettings();

  pref_change_registrar_.Init(prefs_);
  // The following preferences are only used to indicate if a
  // default-content-setting is managed and to hold the managed default-setting
  // value. If the value for any of the following perferences is set then the
  // corresponding default-content-setting is managed. These preferences exist
  // in parallel to the preference default-content-settings.  If a
  // default-content-settings-type is managed any user defined excpetions
  // (patterns) for this type are ignored.
  pref_change_registrar_.Add(prefs::kManagedDefaultCookiesSetting, this);
  pref_change_registrar_.Add(prefs::kManagedDefaultImagesSetting, this);
  pref_change_registrar_.Add(prefs::kManagedDefaultJavaScriptSetting, this);
  pref_change_registrar_.Add(prefs::kManagedDefaultPluginsSetting, this);
  pref_change_registrar_.Add(prefs::kManagedDefaultPopupsSetting, this);
  pref_change_registrar_.Add(prefs::kManagedDefaultGeolocationSetting, this);
  pref_change_registrar_.Add(prefs::kManagedDefaultNotificationsSetting, this);
  pref_change_registrar_.Add(
      prefs::kManagedDefaultAutoSelectCertificateSetting, this);
}

PolicyDefaultProvider::~PolicyDefaultProvider() {
  DCHECK(!prefs_);
}

ContentSetting PolicyDefaultProvider::ProvideDefaultSetting(
    ContentSettingsType content_type) const {
  base::AutoLock auto_lock(lock_);
  return managed_default_content_settings_.settings[content_type];
}

void PolicyDefaultProvider::UpdateDefaultSetting(
    ContentSettingsType content_type,
    ContentSetting setting) {
}

bool PolicyDefaultProvider::DefaultSettingIsManaged(
    ContentSettingsType content_type) const {
  base::AutoLock lock(lock_);
  if (managed_default_content_settings_.settings[content_type] !=
      CONTENT_SETTING_DEFAULT) {
    return true;
  } else {
    return false;
  }
}

void PolicyDefaultProvider::Observe(int type,
                                    const NotificationSource& source,
                                    const NotificationDetails& details) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  if (type == chrome::NOTIFICATION_PREF_CHANGED) {
    DCHECK_EQ(prefs_, Source<PrefService>(source).ptr());
    std::string* name = Details<std::string>(details).ptr();
    if (*name == prefs::kManagedDefaultCookiesSetting) {
      UpdateManagedDefaultSetting(CONTENT_SETTINGS_TYPE_COOKIES);
    } else if (*name == prefs::kManagedDefaultImagesSetting) {
      UpdateManagedDefaultSetting(CONTENT_SETTINGS_TYPE_IMAGES);
    } else if (*name == prefs::kManagedDefaultJavaScriptSetting) {
      UpdateManagedDefaultSetting(CONTENT_SETTINGS_TYPE_JAVASCRIPT);
    } else if (*name == prefs::kManagedDefaultPluginsSetting) {
      UpdateManagedDefaultSetting(CONTENT_SETTINGS_TYPE_PLUGINS);
    } else if (*name == prefs::kManagedDefaultPopupsSetting) {
      UpdateManagedDefaultSetting(CONTENT_SETTINGS_TYPE_POPUPS);
    } else if (*name == prefs::kManagedDefaultGeolocationSetting) {
      UpdateManagedDefaultSetting(CONTENT_SETTINGS_TYPE_GEOLOCATION);
    } else if (*name == prefs::kManagedDefaultNotificationsSetting) {
      UpdateManagedDefaultSetting(CONTENT_SETTINGS_TYPE_NOTIFICATIONS);
    } else if (*name == prefs::kManagedDefaultAutoSelectCertificateSetting) {
      UpdateManagedDefaultSetting(
          CONTENT_SETTINGS_TYPE_AUTO_SELECT_CERTIFICATE);
    } else {
      NOTREACHED() << "Unexpected preference observed";
      return;
    }

    NotifyObservers(ContentSettingsPattern(),
                    ContentSettingsPattern(),
                    CONTENT_SETTINGS_TYPE_DEFAULT,
                    std::string());
  } else {
    NOTREACHED() << "Unexpected notification";
  }
}

void PolicyDefaultProvider::ShutdownOnUIThread() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(prefs_);
  RemoveAllObservers();
  pref_change_registrar_.RemoveAll();
  prefs_ = NULL;
}

void PolicyDefaultProvider::ReadManagedDefaultSettings() {
  for (size_t type = 0; type < arraysize(kPrefToManageType); ++type) {
    if (kPrefToManageType[type] == NULL) {
      continue;
    }
    UpdateManagedDefaultSetting(ContentSettingsType(type));
  }
}

void PolicyDefaultProvider::UpdateManagedDefaultSetting(
    ContentSettingsType type) {
  // If a pref to manage a default-content-setting was not set (NOTICE:
  // "HasPrefPath" returns false if no value was set for a registered pref) then
  // the default value of the preference is used. The default value of a
  // preference to manage a default-content-settings is CONTENT_SETTING_DEFAULT.
  // This indicates that no managed value is set. If a pref was set, than it
  // MUST be managed.
  DCHECK(!prefs_->HasPrefPath(kPrefToManageType[type]) ||
          prefs_->IsManagedPreference(kPrefToManageType[type]));
  base::AutoLock auto_lock(lock_);
  managed_default_content_settings_.settings[type] = IntToContentSetting(
      prefs_->GetInteger(kPrefToManageType[type]));
}

// static
void PolicyDefaultProvider::RegisterUserPrefs(PrefService* prefs) {
  // Preferences for default content setting policies. A policy is not set of
  // the corresponding preferences below is set to CONTENT_SETTING_DEFAULT.
  prefs->RegisterIntegerPref(prefs::kManagedDefaultCookiesSetting,
                             CONTENT_SETTING_DEFAULT,
                             PrefService::UNSYNCABLE_PREF);
  prefs->RegisterIntegerPref(prefs::kManagedDefaultImagesSetting,
                             CONTENT_SETTING_DEFAULT,
                             PrefService::UNSYNCABLE_PREF);
  prefs->RegisterIntegerPref(prefs::kManagedDefaultJavaScriptSetting,
                             CONTENT_SETTING_DEFAULT,
                             PrefService::UNSYNCABLE_PREF);
  prefs->RegisterIntegerPref(prefs::kManagedDefaultPluginsSetting,
                             CONTENT_SETTING_DEFAULT,
                             PrefService::UNSYNCABLE_PREF);
  prefs->RegisterIntegerPref(prefs::kManagedDefaultPopupsSetting,
                             CONTENT_SETTING_DEFAULT,
                             PrefService::UNSYNCABLE_PREF);
  prefs->RegisterIntegerPref(prefs::kManagedDefaultGeolocationSetting,
                             CONTENT_SETTING_DEFAULT,
                             PrefService::UNSYNCABLE_PREF);
  prefs->RegisterIntegerPref(prefs::kManagedDefaultNotificationsSetting,
                             CONTENT_SETTING_DEFAULT,
                             PrefService::UNSYNCABLE_PREF);
  prefs->RegisterIntegerPref(prefs::kManagedDefaultAutoSelectCertificateSetting,
                             CONTENT_SETTING_ASK,
                             PrefService::UNSYNCABLE_PREF);
}

// ////////////////////////////////////////////////////////////////////////////
// PolicyProvider

// static
void PolicyProvider::RegisterUserPrefs(PrefService* prefs) {
  prefs->RegisterListPref(prefs::kManagedAutoSelectCertificateForUrls,
                          PrefService::UNSYNCABLE_PREF);
  prefs->RegisterListPref(prefs::kManagedCookiesAllowedForUrls,
                          PrefService::UNSYNCABLE_PREF);
  prefs->RegisterListPref(prefs::kManagedCookiesBlockedForUrls,
                          PrefService::UNSYNCABLE_PREF);
  prefs->RegisterListPref(prefs::kManagedCookiesSessionOnlyForUrls,
                          PrefService::UNSYNCABLE_PREF);
  prefs->RegisterListPref(prefs::kManagedImagesAllowedForUrls,
                          PrefService::UNSYNCABLE_PREF);
  prefs->RegisterListPref(prefs::kManagedImagesBlockedForUrls,
                          PrefService::UNSYNCABLE_PREF);
  prefs->RegisterListPref(prefs::kManagedJavaScriptAllowedForUrls,
                          PrefService::UNSYNCABLE_PREF);
  prefs->RegisterListPref(prefs::kManagedJavaScriptBlockedForUrls,
                          PrefService::UNSYNCABLE_PREF);
  prefs->RegisterListPref(prefs::kManagedPluginsAllowedForUrls,
                          PrefService::UNSYNCABLE_PREF);
  prefs->RegisterListPref(prefs::kManagedPluginsBlockedForUrls,
                          PrefService::UNSYNCABLE_PREF);
  prefs->RegisterListPref(prefs::kManagedPopupsAllowedForUrls,
                          PrefService::UNSYNCABLE_PREF);
  prefs->RegisterListPref(prefs::kManagedPopupsBlockedForUrls,
                          PrefService::UNSYNCABLE_PREF);
}

PolicyProvider::PolicyProvider(PrefService* prefs,
                               DefaultProviderInterface* default_provider)
    : prefs_(prefs),
      default_provider_(default_provider) {
  ReadManagedContentSettings(false);

  pref_change_registrar_.Init(prefs_);
  pref_change_registrar_.Add(prefs::kManagedAutoSelectCertificateForUrls, this);
  pref_change_registrar_.Add(prefs::kManagedCookiesBlockedForUrls, this);
  pref_change_registrar_.Add(prefs::kManagedCookiesAllowedForUrls, this);
  pref_change_registrar_.Add(prefs::kManagedCookiesSessionOnlyForUrls, this);
  pref_change_registrar_.Add(prefs::kManagedImagesBlockedForUrls, this);
  pref_change_registrar_.Add(prefs::kManagedImagesAllowedForUrls, this);
  pref_change_registrar_.Add(prefs::kManagedJavaScriptBlockedForUrls, this);
  pref_change_registrar_.Add(prefs::kManagedJavaScriptAllowedForUrls, this);
  pref_change_registrar_.Add(prefs::kManagedPluginsBlockedForUrls, this);
  pref_change_registrar_.Add(prefs::kManagedPluginsAllowedForUrls, this);
  pref_change_registrar_.Add(prefs::kManagedPopupsBlockedForUrls, this);
  pref_change_registrar_.Add(prefs::kManagedPopupsAllowedForUrls, this);
}

PolicyProvider::~PolicyProvider() {
  DCHECK(!prefs_);
}

void PolicyProvider::GetContentSettingsFromPreferences(
    OriginIdentifierValueMap* value_map) {
  for (size_t i = 0; i < arraysize(kPrefsForManagedContentSettingsMap); ++i) {
    const char* pref_name = kPrefsForManagedContentSettingsMap[i].pref_name;
    // Skip unset policies.
    if (!prefs_->HasPrefPath(pref_name)) {
      VLOG(2) << "Skipping unset preference: " << pref_name;
      continue;
    }

    const PrefService::Preference* pref = prefs_->FindPreference(pref_name);
    DCHECK(pref);
    DCHECK(pref->IsManaged());

    const ListValue* pattern_str_list = NULL;
    if (!pref->GetValue()->GetAsList(&pattern_str_list)) {
      NOTREACHED();
      return;
    }

    for (size_t j = 0; j < pattern_str_list->GetSize(); ++j) {
      std::string original_pattern_str;
      pattern_str_list->GetString(j, &original_pattern_str);
      PatternPair pattern_pair = ParsePatternString(original_pattern_str);
      // Ignore invalid patterns.
      if (!pattern_pair.first.IsValid()) {
        VLOG(1) << "Ignoring invalid content settings pattern: " <<
                   original_pattern_str;
        continue;
      }

      ContentSettingsType content_type =
          kPrefsForManagedContentSettingsMap[i].content_type;
      // If only one pattern was defined auto expand it to a pattern pair.
      ContentSettingsPattern secondary_pattern =
          !pattern_pair.second.IsValid() ? ContentSettingsPattern::Wildcard()
                                         : pattern_pair.second;
      value_map->SetValue(
          pattern_pair.first,
          secondary_pattern,
          content_type,
          ResourceIdentifier(NO_RESOURCE_IDENTIFIER),
          static_cast<Value*>(Value::CreateIntegerValue(
              kPrefsForManagedContentSettingsMap[i].setting)));
    }
  }
}

void PolicyProvider::ReadManagedContentSettings(bool overwrite) {
  {
    base::AutoLock auto_lock(lock_);
    if (overwrite)
      value_map_.clear();
    GetContentSettingsFromPreferences(&value_map_);
  }
}

// Since the PolicyProvider is a read only content settings provider, all
// methodes of the ProviderInterface that set or delete any settings do nothing.
void PolicyProvider::SetContentSetting(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    ContentSettingsType content_type,
    const ResourceIdentifier& resource_identifier,
    ContentSetting content_setting) {
}

ContentSetting PolicyProvider::GetContentSetting(
    const GURL& primary_url,
    const GURL& secondary_url,
    ContentSettingsType content_type,
    const ResourceIdentifier& resource_identifier) const {
  // Resource identifier are not supported by policies as long as the feature is
  // behind a flag. So resource identifiers are simply ignored.
  Value* value = value_map_.GetValue(primary_url,
                                     secondary_url,
                                     content_type,
                                     resource_identifier);
  ContentSetting setting =
      value == NULL ? CONTENT_SETTING_DEFAULT : ValueToContentSetting(value);
  if (setting == CONTENT_SETTING_DEFAULT && default_provider_)
    setting = default_provider_->ProvideDefaultSetting(content_type);
  return setting;
}

void PolicyProvider::GetAllContentSettingsRules(
    ContentSettingsType content_type,
    const ResourceIdentifier& resource_identifier,
    Rules* content_setting_rules) const {
  DCHECK_NE(RequiresResourceIdentifier(content_type),
            resource_identifier.empty());
  DCHECK(content_setting_rules);
  content_setting_rules->clear();

  const OriginIdentifierValueMap* map_to_return = &value_map_;

  base::AutoLock auto_lock(lock_);
  for (OriginIdentifierValueMap::const_iterator entry = map_to_return->begin();
       entry != map_to_return->end();
       ++entry) {
    if (entry->content_type == content_type &&
        entry->identifier == resource_identifier) {
      ContentSetting setting = ValueToContentSetting(entry->value.get());
      DCHECK(setting != CONTENT_SETTING_DEFAULT);
      Rule new_rule(entry->primary_pattern,
                    entry->secondary_pattern,
                    setting);
      content_setting_rules->push_back(new_rule);
    }
  }
}

void PolicyProvider::ClearAllContentSettingsRules(
    ContentSettingsType content_type) {
}

void PolicyProvider::ShutdownOnUIThread() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  RemoveAllObservers();
  if (!prefs_)
    return;
  pref_change_registrar_.RemoveAll();
  prefs_ = NULL;
}

void PolicyProvider::Observe(int type,
                             const NotificationSource& source,
                             const NotificationDetails& details) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  if (type == chrome::NOTIFICATION_PREF_CHANGED) {
    DCHECK_EQ(prefs_, Source<PrefService>(source).ptr());
    std::string* name = Details<std::string>(details).ptr();
    if (*name == prefs::kManagedAutoSelectCertificateForUrls ||
        *name == prefs::kManagedCookiesAllowedForUrls ||
        *name == prefs::kManagedCookiesBlockedForUrls ||
        *name == prefs::kManagedCookiesSessionOnlyForUrls ||
        *name == prefs::kManagedImagesAllowedForUrls ||
        *name == prefs::kManagedImagesBlockedForUrls ||
        *name == prefs::kManagedJavaScriptAllowedForUrls ||
        *name == prefs::kManagedJavaScriptBlockedForUrls ||
        *name == prefs::kManagedPluginsAllowedForUrls ||
        *name == prefs::kManagedPluginsBlockedForUrls ||
        *name == prefs::kManagedPopupsAllowedForUrls ||
        *name == prefs::kManagedPopupsBlockedForUrls) {
      ReadManagedContentSettings(true);
      NotifyObservers(ContentSettingsPattern(),
                      ContentSettingsPattern(),
                      CONTENT_SETTINGS_TYPE_DEFAULT,
                      std::string());
    }
  } else {
    NOTREACHED() << "Unexpected notification";
  }
}

}  // namespace content_settings
