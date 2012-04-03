// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/ui/idle_logout_dialog_view.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/time.h"
#include "base/string_number_conversions.h"
#include "chrome/browser/chromeos/kiosk_mode/kiosk_mode_settings.h"
#include "chrome/browser/ui/browser_list.h"
#include "grit/browser_resources.h"
#include "grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/layout/layout_constants.h"
#include "ui/views/widget/widget.h"

namespace {

// Global singleton instance of our dialog class.
chromeos::IdleLogoutDialogView* g_instance = NULL;

const int kIdleLogoutDialogMaxWidth = 400;
const int kCountdownUpdateIntervalMs = 1000;

}  // namespace

namespace chromeos {

IdleLogoutSettingsProvider* IdleLogoutDialogView::provider_ = NULL;

////////////////////////////////////////////////////////////////////////////////
// IdleLogoutSettingsProvider public methods
IdleLogoutSettingsProvider::IdleLogoutSettingsProvider() {
}

IdleLogoutSettingsProvider::~IdleLogoutSettingsProvider() {
}

base::TimeDelta IdleLogoutSettingsProvider::GetCountdownUpdateInterval() {
  return base::TimeDelta::FromMilliseconds(kCountdownUpdateIntervalMs);
}

KioskModeSettings* IdleLogoutSettingsProvider::GetKioskModeSettings() {
  return KioskModeSettings::Get();
}

void IdleLogoutSettingsProvider::LogoutCurrentUser(IdleLogoutDialogView*) {
  BrowserList::AttemptUserExit();
}

////////////////////////////////////////////////////////////////////////////////
// IdleLogoutDialogView public static methods
// static
void IdleLogoutDialogView::ShowDialog() {
  // We only show the dialog if it is not already showing. We don't want two
  // countdowns on the screen for any reason. If the dialog is closed by using
  // CloseDialog, we reset g_instance so the next Show will work correctly; in
  // case the dialog is closed by the system, DeleteDelegate is guaranteed to be
  // called, in which case we reset g_instance there if not already reset.
  if (!g_instance) {
    g_instance = new IdleLogoutDialogView();
    g_instance->Init();
    g_instance->Show();
  }
}

// static
void IdleLogoutDialogView::CloseDialog() {
  if (g_instance) {
    g_instance->set_closed();
    g_instance->Close();
    g_instance = NULL;
  }
}

////////////////////////////////////////////////////////////////////////////////
// Overridden from views::DialogDelegateView
int IdleLogoutDialogView::GetDialogButtons() const {
  return ui::DIALOG_BUTTON_NONE;
}

ui::ModalType IdleLogoutDialogView::GetModalType() const {
  return ui::MODAL_TYPE_WINDOW;
}

string16 IdleLogoutDialogView::GetWindowTitle() const {
  return l10n_util::GetStringUTF16(IDS_IDLE_LOGOUT_TITLE);
}

views::View* IdleLogoutDialogView::GetContentsView() {
  return this;
}

void IdleLogoutDialogView::DeleteDelegate() {
  // There isn't a delegate method that is called on close and is 'not' called
  // async; this can cause an issue with us setting the g_instance to NULL
  // 'after' another Show call has been called on it. So instead, we rely on
  // CloseIdleLogoutDialog to set g_instance to NULL; in the case that we get
  // closed by any other way than CloseIdleLogoutDialog, we check if our
  // 'closed' state is set - if not, then we set it and set g_instance to null,
  // since that means that CloseIdleLogoutDialog was never called.
  if (!this->is_closed()) {
    g_instance->set_closed();
    g_instance = NULL;
  }

  delete this;
}

////////////////////////////////////////////////////////////////////////////////
// IdleLogoutDialog private methods
IdleLogoutDialogView::IdleLogoutDialogView()
    : restart_label_(NULL),
      warning_label_(NULL),
      weak_ptr_factory_(ALLOW_THIS_IN_INITIALIZER_LIST(this)) {
  if (!IdleLogoutDialogView::provider_)
    IdleLogoutDialogView::provider_ = new IdleLogoutSettingsProvider();
}

IdleLogoutDialogView::~IdleLogoutDialogView() {
}

void IdleLogoutDialogView::Init() {
  KioskModeSettings* settings =
      IdleLogoutDialogView::provider_->GetKioskModeSettings();
  if (!settings->is_initialized()) {
    settings->Initialize(base::Bind(&IdleLogoutDialogView::Init,
                                    weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();

  warning_label_ = new views::Label(
      l10n_util::GetStringUTF16(IDS_IDLE_LOGOUT_WARNING));
  warning_label_->SetHorizontalAlignment(views::Label::ALIGN_LEFT);
  warning_label_->SetMultiLine(true);
  warning_label_->SetFont(rb.GetFont(ui::ResourceBundle::BaseFont));
  warning_label_->SizeToFit(kIdleLogoutDialogMaxWidth);

  restart_label_ = new views::Label();
  restart_label_->SetHorizontalAlignment(views::Label::ALIGN_LEFT);
  restart_label_->SetFont(rb.GetFont(ui::ResourceBundle::BoldFont));

  views::GridLayout* layout = views::GridLayout::CreatePanel(this);
  SetLayoutManager(layout);

  views::ColumnSet* column_set = layout->AddColumnSet(0);
  column_set->AddColumn(views::GridLayout::FILL, views::GridLayout::CENTER, 1,
                        views::GridLayout::USE_PREF, 0, 0);
  layout->StartRow(0, 0);
  layout->AddView(warning_label_);
  layout->AddPaddingRow(0, views::kUnrelatedControlVerticalSpacing);
  layout->StartRow(0, 0);
  layout->AddView(restart_label_);
}

void IdleLogoutDialogView::Show() {
  KioskModeSettings* settings =
        IdleLogoutDialogView::provider_->GetKioskModeSettings();

  // Setup the countdown label before showing.
  countdown_end_time_ = base::Time::Now() +
      settings->GetIdleLogoutWarningDuration();

  UpdateCountdown();

  views::Widget::CreateWindow(this);
  GetWidget()->SetAlwaysOnTop(true);
  GetWidget()->Show();

  // Update countdown every 1 second.
  timer_.Start(FROM_HERE,
               IdleLogoutDialogView::provider_->GetCountdownUpdateInterval(),
               this,
               &IdleLogoutDialogView::UpdateCountdown);
}

void IdleLogoutDialogView::Close() {
  DCHECK(GetWidget());

  if (timer_.IsRunning())
    timer_.Stop();
  GetWidget()->Close();
}

void IdleLogoutDialogView::UpdateCountdown() {
  base::TimeDelta logout_warning_time = countdown_end_time_ -
                                        base::Time::Now();
  int64 seconds_left = (logout_warning_time.InMillisecondsF() /
                        base::Time::kMillisecondsPerSecond) + 0.5;

  if (seconds_left > 1) {
    restart_label_->SetText(l10n_util::GetStringFUTF16(
        IDS_IDLE_LOGOUT_WARNING_RESTART,
        base::Int64ToString16(seconds_left)));
  } else if (seconds_left > 0) {
    restart_label_->SetText(l10n_util::GetStringUTF16(
        IDS_IDLE_LOGOUT_WARNING_RESTART_1S));
  } else {
    // Set the label - the logout probably won't be instant.
    restart_label_->SetText(l10n_util::GetStringUTF16(
        IDS_IDLE_LOGOUT_WARNING_RESTART_NOW));

    // We're done; stop the timer and logout.
    timer_.Stop();
    IdleLogoutDialogView::provider_->LogoutCurrentUser(this);
  }
}

// static
IdleLogoutDialogView* IdleLogoutDialogView::current_instance() {
  return g_instance;
}

// static
void IdleLogoutDialogView::set_settings_provider(
    IdleLogoutSettingsProvider* provider) {
  provider_ = provider;
}

}  // namespace chromeos
