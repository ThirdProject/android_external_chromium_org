// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SYNC_PROMO_UI_H_
#define CHROME_BROWSER_UI_WEBUI_SYNC_PROMO_UI_H_
#pragma once

#include "chrome/browser/ui/webui/chrome_web_ui.h"

// The Web UI handler for chrome://syncpromo.
class SyncPromoUI : public ChromeWebUI {
 public:
  // Constructs a SyncPromoUI. |contents| is the TabContents that this WebUI is
  // associated with. |contents| may not be NULL.
  explicit SyncPromoUI(TabContents* contents);

  // Returns true if the sync promo should be visible.
  static bool ShouldShowSyncPromo();

 private:
  DISALLOW_COPY_AND_ASSIGN(SyncPromoUI);
};

#endif  // CHROME_BROWSER_UI_WEBUI_SYNC_PROMO_UI_H_
