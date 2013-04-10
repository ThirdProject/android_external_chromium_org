// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROME_BROWSER_MAIN_EXTRA_PARTS_H_
#define CHROME_BROWSER_CHROME_BROWSER_MAIN_EXTRA_PARTS_H_

// Interface class for Parts owned by ChromeBrowserMainParts.
// The default implementation for all methods is empty.

// Most of these map to content::BrowserMainParts methods. This interface is
// separate to allow stages to be further subdivided for Chrome specific
// initialization stages (e.g. browser process init, profile init).

// While ChromeBrowserMainParts are platform-specific,
// ChromeBrowserMainExtraParts are used to do further initialization for various
// Chrome toolkits (e.g., GTK, VIEWS, ASH, AURA, etc.; see
// ChromeContentBrowserClient::CreateBrowserMainParts()).

class ChromeBrowserMainExtraParts {
 public:
  virtual ~ChromeBrowserMainExtraParts() {}

  // EarlyInitialization methods.
  virtual void PreEarlyInitialization() {}
  virtual void PostEarlyInitialization() {}

  // ToolkitInitialized methods.
  virtual void ToolkitInitialized() {}

  // MainMessageLoopStart methods.
  virtual void PreMainMessageLoopStart() {}
  virtual void PostMainMessageLoopStart() {}

  // MainMessageLoopRun methods.
  virtual void PreCreateThreads() {}
  virtual void PreProfileInit() {}
  virtual void PostProfileInit() {}
  virtual void PreBrowserStart() {}
  virtual void PostBrowserStart() {}
  virtual void PreMainMessageLoopRun() {}
  virtual void PostMainMessageLoopRun() {}
};

#endif  // CHROME_BROWSER_CHROME_BROWSER_MAIN_EXTRA_PARTS_H_
