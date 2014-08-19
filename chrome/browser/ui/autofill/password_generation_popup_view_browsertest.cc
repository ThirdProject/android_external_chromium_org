// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/autofill/password_generation_popup_view.h"

#include "base/strings/string16.h"
#include "chrome/browser/ui/autofill/password_generation_popup_controller_impl.h"
#include "chrome/browser/ui/autofill/password_generation_popup_view_tester.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/event_utils.h"

namespace autofill {

class TestPasswordGenerationPopupController :
      public PasswordGenerationPopupControllerImpl {
 public:
  TestPasswordGenerationPopupController(content::WebContents* web_contents,
                                        gfx::NativeView native_view)
      : PasswordGenerationPopupControllerImpl(
            gfx::RectF(0, 0, 10, 10),
            PasswordForm(),
            10,
            NULL /* PasswordManager*/,
            NULL /* PasswordGenerationPopupObserver*/,
            web_contents,
            native_view) {}

  virtual ~TestPasswordGenerationPopupController() {}

  PasswordGenerationPopupView* view() {
    return view_;
  }
};

class PasswordGenerationPopupViewTest : public InProcessBrowserTest {
 public:
  virtual void SetUpOnMainThread() OVERRIDE {
    gfx::NativeView native_view =
        browser()->tab_strip_model()->GetActiveWebContents()->GetNativeView();

    controller_ =
        new TestPasswordGenerationPopupController(
            browser()->tab_strip_model()->GetActiveWebContents(), native_view);
  }

  scoped_ptr<PasswordGenerationPopupViewTester> GetViewTester() {
    return PasswordGenerationPopupViewTester::For(controller_->view()).Pass();
  }

 protected:
  TestPasswordGenerationPopupController* controller_;
};

// TODO(gcasto): Enable on Mac when UI is updated. (crbug.com/394303)
#if !defined(OS_MACOSX)
// Regression test for crbug.com/400543. Verifying that moving the mouse in the
// editing dialog doesn't crash.
IN_PROC_BROWSER_TEST_F(PasswordGenerationPopupViewTest,
                       MouseMovementInEditingPopup) {
  controller_->Show(false /* display_password */);

  gfx::Point center_point =
      static_cast<PasswordGenerationPopupController*>(
          controller_)->popup_bounds().CenterPoint();
  GetViewTester()->SimulateMouseMovementAt(center_point);

  // Deletes |controller_|.
  controller_->HideAndDestroy();
}
#endif

}  // namespace autofill
