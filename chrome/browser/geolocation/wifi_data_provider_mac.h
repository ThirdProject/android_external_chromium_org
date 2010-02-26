// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_GEOLOCATION_WIFI_DATA_PROVIDER_MAC_H_
#define CHROME_BROWSER_GEOLOCATION_WIFI_DATA_PROVIDER_MAC_H_

#include "chrome/browser/geolocation/wifi_data_provider_common.h"

class OsxWifiDataProvider : public WifiDataProviderCommon {
 public:
  OsxWifiDataProvider();

 private:
  virtual ~OsxWifiDataProvider();

  // WifiDataProviderCommon
  virtual WlanApiInterface* NewWlanApi();
  virtual PollingPolicyInterface* NewPollingPolicy();

  DISALLOW_COPY_AND_ASSIGN(OsxWifiDataProvider);
};

#endif  // CHROME_BROWSER_GEOLOCATION_WIFI_DATA_PROVIDER_MAC_H_
