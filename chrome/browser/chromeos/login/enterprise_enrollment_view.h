// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_ENTERPRISE_ENROLLMENT_VIEW_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_ENTERPRISE_ENROLLMENT_VIEW_H_
#pragma once

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "chrome/browser/chromeos/login/web_page_view.h"
#include "chrome/browser/ui/webui/chromeos/enterprise_enrollment_ui.h"
#include "views/view.h"

namespace views {
class GridLayout;
class Label;
}

namespace chromeos {

class EnterpriseEnrollmentController;
class ScreenObserver;

// Implements the UI for the enterprise enrollment screen in OOBE.
class EnterpriseEnrollmentView : public views::View,
                                 public EnterpriseEnrollmentUI::Controller {
 public:
  explicit EnterpriseEnrollmentView(EnterpriseEnrollmentController* controller);
  virtual ~EnterpriseEnrollmentView();

  // Initialize view controls and layout.
  void Init();

  // Switches to the confirmation screen.
  void ShowConfirmationScreen();

  // EnterpriseEnrollmentUI::Controller implementation.
  virtual void OnAuthSubmitted(const std::string& user,
                               const std::string& password,
                               const std::string& captcha,
                               const std::string& access_code) OVERRIDE;
  virtual void OnAuthCancelled() OVERRIDE;
  virtual void OnConfirmationClosed() OVERRIDE;

 private:
  // Overriden from views::View:
  virtual void Layout() OVERRIDE;

  EnterpriseEnrollmentController* controller_;

  // Controls.
  WebPageDomView* enrollment_page_view_;

  DISALLOW_COPY_AND_ASSIGN(EnterpriseEnrollmentView);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_ENTERPRISE_ENROLLMENT_VIEW_H_
