// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_ASH_CHROME_BROWSER_MAIN_EXTRA_PARTS_ASH_H_
#define CHROME_BROWSER_UI_VIEWS_ASH_CHROME_BROWSER_MAIN_EXTRA_PARTS_ASH_H_
#pragma once

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "chrome/browser/chrome_browser_main_extra_parts.h"

namespace aura {
namespace client {
class StackingClient;
}
}

class ChromeBrowserMainExtraPartsAsh : public ChromeBrowserMainExtraParts {
 public:
  ChromeBrowserMainExtraPartsAsh();
  virtual ~ChromeBrowserMainExtraPartsAsh();

  // Overridden from ChromeBrowserMainExtraParts:
  virtual void PreProfileInit() OVERRIDE;
  virtual void PostProfileInit() OVERRIDE;
  virtual void PostMainMessageLoopRun() OVERRIDE;

 private:
  scoped_ptr<aura::client::StackingClient> stacking_client_;

  DISALLOW_COPY_AND_ASSIGN(ChromeBrowserMainExtraPartsAsh);
};

#endif  // CHROME_BROWSER_UI_VIEWS_ASH_CHROME_BROWSER_MAIN_EXTRA_PARTS_ASH_H_
