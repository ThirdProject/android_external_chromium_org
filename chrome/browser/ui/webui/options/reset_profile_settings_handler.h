// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_OPTIONS_RESET_PROFILE_SETTINGS_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_OPTIONS_RESET_PROFILE_SETTINGS_HANDLER_H_

#include "base/compiler_specific.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/ui/webui/options/options_ui.h"

namespace base {
class DictionaryValue;
class ListValue;
}  // namespace base

class AutomaticProfileResetter;
class BrandcodeConfigFetcher;
class ProfileResetter;
class ResettableSettingsSnapshot;

namespace options {

// Handler for both the 'Reset Profile Settings' overlay page and also the
// corresponding banner that is shown at the top of the options page.
class ResetProfileSettingsHandler
    : public OptionsPageUIHandler,
      public base::SupportsWeakPtr<ResetProfileSettingsHandler> {
 public:
  ResetProfileSettingsHandler();
  virtual ~ResetProfileSettingsHandler();

  // OptionsPageUIHandler implementation.
  virtual void GetLocalizedValues(
      base::DictionaryValue* localized_strings) OVERRIDE;
  virtual void InitializeHandler() OVERRIDE;
  virtual void InitializePage() OVERRIDE;
  virtual void Uninitialize() OVERRIDE;

  // WebUIMessageHandler implementation.
  virtual void RegisterMessages() OVERRIDE;

 private:
  // Javascript callback to start clearing data.
  void HandleResetProfileSettings(const base::ListValue* value);

  // Closes the dialog once all requested settings has been reset.
  void OnResetProfileSettingsDone();

  // Called when the confirmation box appears.
  void OnShowResetProfileDialog(const base::ListValue* value);

  // Called when the reset banner is dismissed from the WebUI.
  void OnDismissedResetProfileSettingsBanner(const base::ListValue* args);

  // Called when BrandcodeConfigFetcher completed fetching settings.
  void OnSettingsFetched();

  // Resets profile settings to default values. |send_settings| is true if user
  // gave his consent to upload broken settings to Google for analysis.
  void ResetProfile(bool send_settings);

  // Destroyed with the Profile, thus it should outlive us. This will be NULL if
  // the underlying profile is off-the-record (e.g. in Guest mode on Chrome OS).
  AutomaticProfileResetter* automatic_profile_resetter_;

  // Records whether or not the Profile Reset confirmation dialog was opened at
  // least once during the lifetime of the settings page.
  bool has_shown_confirmation_dialog_;

  scoped_ptr<ProfileResetter> resetter_;

  scoped_ptr<BrandcodeConfigFetcher> config_fetcher_;

  // Snapshot of settings before profile was reseted.
  scoped_ptr<ResettableSettingsSnapshot> setting_snapshot_;

  // Contains Chrome brand code; empty for organic Chrome.
  std::string brandcode_;

  DISALLOW_COPY_AND_ASSIGN(ResetProfileSettingsHandler);
};

}  // namespace options

#endif  // CHROME_BROWSER_UI_WEBUI_OPTIONS_RESET_PROFILE_SETTINGS_HANDLER_H_
