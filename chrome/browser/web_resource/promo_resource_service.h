// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_WEB_RESOURCE_PROMO_RESOURCE_SERVICE_H_
#define CHROME_BROWSER_WEB_RESOURCE_PROMO_RESOURCE_SERVICE_H_
#pragma once

#include <string>

#include "base/memory/weak_ptr.h"
#include "chrome/browser/web_resource/notification_promo.h"
#include "chrome/browser/web_resource/web_resource_service.h"
#include "chrome/common/chrome_version_info.h"

namespace base {
class DictionaryValue;
}

class AppsPromoLogoFetcher;
class PrefService;
class Profile;

// A PromoResourceService fetches data from a web resource server to be used to
// dynamically change the appearance of the New Tab Page. For example, it has
// been used to fetch "tips" to be displayed on the NTP, or to display
// promotional messages to certain groups of Chrome users.
class PromoResourceService : public WebResourceService {
 public:
  // Identifies types of Chrome builds for promo targeting.
  enum BuildType {
    NO_BUILD = 0,
    DEV_BUILD = 1,
    BETA_BUILD = 1 << 1,
    STABLE_BUILD = 1 << 2,
    CANARY_BUILD = 1 << 3,
    ALL_BUILDS = (1 << 4) - 1,
  };

  // Checks for conditions to show promo: start/end times, channel, etc.
  static bool CanShowNotificationPromo(Profile* profile);

  static void RegisterPrefs(PrefService* local_state);

  static void RegisterUserPrefs(PrefService* prefs);

  explicit PromoResourceService(Profile* profile);

  static chrome::VersionInfo::Channel GetChannel();
  static bool IsBuildTargeted(chrome::VersionInfo::Channel, int builds_allowed);

 private:
  FRIEND_TEST_ALL_PREFIXES(PromoResourceServiceTest, IsBuildTargetedTest);
  FRIEND_TEST_ALL_PREFIXES(PromoResourceServiceTest, UnpackLogoSignal);
  FRIEND_TEST_ALL_PREFIXES(PromoResourceServiceTest, UnpackWebStoreSignal);
  FRIEND_TEST_ALL_PREFIXES(
      PromoResourceServiceTest, UnpackPartialWebStoreSignal);
  FRIEND_TEST_ALL_PREFIXES(
      PromoResourceServiceTest, UnpackWebStoreSignalHttpsLogo);
  FRIEND_TEST_ALL_PREFIXES(
      PromoResourceServiceTest, UnpackWebStoreSignalHttpsLogoError);
  FRIEND_TEST_ALL_PREFIXES(
      PromoResourceServiceTest, UnpackWebStoreSignalHttpLogo);

  virtual ~PromoResourceService();

  int GetPromoServiceVersion();

  // Gets the locale of the last promos fetched from the server. This is saved
  // so we can fetch new data if the locale changes.
  std::string GetPromoLocale();

  // Returns true if |builds_targeted| includes the release channel Chrome
  // belongs to. For testing purposes, you can override the current channel
  // with set_channel.
  bool IsBuildTargeted(int builds_targeted);

  // Schedule a notification that a web resource is either going to become
  // available or be no longer valid.
  void ScheduleNotification(double start, double end);

  // Schedules the initial notification for when the web resource is going
  // to become available or no longer valid. This performs a few additional
  // checks than ScheduleNotification, namely it schedules updates immediately
  // if the promo service or Chrome locale has changed.
  void ScheduleNotificationOnInit();

  // If delay_ms is positive, schedule notification with the delay.
  // If delay_ms is 0, notify immediately by calling WebResourceStateChange().
  // If delay_ms is negative, do nothing.
  void PostNotification(int64 delay_ms);

  // Notify listeners that the state of a web resource has changed.
  void PromoResourceStateChange();

  // Overrides the current Chrome release channel for testing purposes.
  void set_channel(chrome::VersionInfo::Channel channel) { channel_ = channel; }

  // WebResourceService override.
  virtual void Unpack(const base::DictionaryValue& parsed_json) OVERRIDE;

  // Unpack the web resource as a custom notification signal.
  void UnpackNotificationSignal(const base::DictionaryValue& parsed_json);

  // Unpack the promo resource as a custom logo signal. Expects a start and end
  // signal. Delivery will be in json in the form of:
  // {
  //   "topic": {
  //     "answers": [
  //       {
  //         "answer_id": "107366",
  //         "name": "custom_logo_start",
  //         "question": "",
  //         "tooltip": "",
  //         "inproduct": "10/8/09 12:00",
  //         "inproduct_target": null
  //       },
  //       {
  //         "answer_id": "107366",
  //         "name": "custom_logo_end",
  //         "question": "",
  //         "tooltip": "",
  //         "inproduct": "10/8/09 12:00",
  //         "inproduct_target": null
  //       },
  //       ...
  //     ]
  //   }
  // }
  //
  void UnpackLogoSignal(const base::DictionaryValue& parsed_json);

  // Unpack the web store promo. Expects JSON delivery in the following format:
  // {
  //   "topic": {
  //     "answers": [
  //       {
  //         "answer_id": "1143011",
  //         "name": "webstore_promo:15:1:https://www.google.com/logo.png",
  //         "question": "Browse thousands of apps and games for Chrome.",
  //         "inproduct_target": "Visit the Chrome Web Store",
  //         "inproduct": "https://chrome.google.com/webstore?hl=en",
  //         "tooltip": "No thanks, hide this"
  //       },
  //       ...
  //     ]
  //   }
  // }
  // The properties are defined as follows:
  //   inproduct: the release channels targeted (bitwise or of BuildTypes)
  //   question: the promo header text
  //   inproduct_target: the promo button text
  //   inproduct: the promo button link
  //   tooltip: the text for the "hide this" link on the promo
  //   name: starts with "webstore_promo" to identify the signal. The second
  //         part contains the release channels targeted (bitwise or of
  //         BuildTypes). The third part specifies what users should maximize
  //         the apps section of the NTP when first loading the promo (bitwise
  //         or of AppsPromo::UserGroup). The forth part is optional and
  //         specifies the URL of the logo image. If left out, the default
  //         webstore logo will be used. The logo can be an HTTPS or DATA URL.
  //   answer_id: the promo's id
  void UnpackWebStoreSignal(const base::DictionaryValue& parsed_json);

  // The profile this service belongs to.
  Profile* profile_;

  // Overrides the current Chrome release channel for testing purposes.
  chrome::VersionInfo::Channel channel_;

  // Allows the creation of tasks to send a notification.
  // This allows the PromoResourceService to notify the New Tab Page immediately
  // when a new web resource should be shown or removed.
  base::WeakPtrFactory<PromoResourceService> weak_ptr_factory_;

  // Notification type when an update is done.
  int notification_type_;

  // True if a task has been set to update the cache when a new web resource
  // becomes available.
  bool web_resource_update_scheduled_;

  // A helper that downloads the promo logo.
  scoped_ptr<AppsPromoLogoFetcher> apps_promo_logo_fetcher_;

  DISALLOW_COPY_AND_ASSIGN(PromoResourceService);
};

#endif  // CHROME_BROWSER_WEB_RESOURCE_PROMO_RESOURCE_SERVICE_H_
