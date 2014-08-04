// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BATTERY_STATUS_BATTERY_STATUS_MANAGER_H_
#define CHROME_BROWSER_BATTERY_STATUS_BATTERY_STATUS_MANAGER_H_

#include "base/memory/scoped_ptr.h"
#include "build/build_config.h"
#include "content/browser/battery_status/battery_status_service.h"

namespace content {

// Platform specific manager class for fetching battery status data.
class CONTENT_EXPORT BatteryStatusManager {
 public:
  // Creates a BatteryStatusManager object. |callback| should be called when the
  // battery status changes.
  static scoped_ptr<BatteryStatusManager> Create(
      const BatteryStatusService::BatteryUpdateCallback& callback);

  virtual ~BatteryStatusManager() {}

  // Start listening for battery status changes. New updates are signalled
  // by invoking the callback provided at construction time.
  // Note that this is called in the IO thread.
  virtual bool StartListeningBatteryChange() = 0;

  // Stop listening for battery status changes.
  // Note that this is called in the IO thread.
  virtual void StopListeningBatteryChange() = 0;
};

}  // namespace content

#endif  // CHROME_BROWSER_BATTERY_STATUS_BATTERY_STATUS_MANAGER_H_
