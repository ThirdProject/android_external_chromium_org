// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/browser_accessibility_state.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/uma_histogram_helper.h"

namespace content {

// Note: even though BrowserAccessibilityStateImpl is in content, this
// test should be in Chrome because otherwise the Chrome-OS-specific
// histograms won't get updated.
class BrowserAccessibilityStateImplTest : public InProcessBrowserTest {
};

IN_PROC_BROWSER_TEST_F(BrowserAccessibilityStateImplTest, TestHistograms) {
  UMAHistogramHelper histograms;

  BrowserAccessibilityState::GetInstance()->UpdateHistogramsForTesting();
  histograms.Fetch();
  histograms.ExpectTotalCount("Accessibility.State", 1);
#if defined(OS_WIN)
  histograms.ExpectTotalCount("Accessibility.WinScreenReader", 1);
#elif defined(OS_CHROMEOS)
  histograms.ExpectTotalCount("Accessibility.CrosSpokenFeedback", 1);
#endif
}

}  // namespace content
