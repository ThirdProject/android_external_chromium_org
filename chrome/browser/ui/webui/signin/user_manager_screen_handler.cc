// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/signin/user_manager_screen_handler.h"

#include "base/bind.h"
#include "base/value_conversions.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/avatar_menu.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_info_cache.h"
#include "chrome/browser/profiles/profile_info_cache_observer.h"
#include "chrome/browser/profiles/profile_info_util.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/profiles/profiles_state.h"
#include "chrome/browser/signin/local_auth.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_view.h"
#include "content/public/browser/web_ui.h"
#include "grit/browser_resources.h"
#include "grit/chromium_strings.h"
#include "grit/generated_resources.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/webui/web_ui_util.h"
#include "ui/gfx/image/image_util.h"

#if defined(ENABLE_MANAGED_USERS)
#include "chrome/browser/managed_mode/managed_user_service.h"
#endif

namespace {
// User dictionary keys.
const char kKeyUsername[] = "username";
const char kKeyDisplayName[]= "displayName";
const char kKeyEmailAddress[] = "emailAddress";
const char kKeyProfilePath[] = "profilePath";
const char kKeyPublicAccount[] = "publicAccount";
const char kKeyLocallyManagedUser[] = "locallyManagedUser";
const char kKeySignedIn[] = "signedIn";
const char kKeyCanRemove[] = "canRemove";
const char kKeyIsOwner[] = "isOwner";
const char kKeyIsDesktop[] = "isDesktopUser";
const char kKeyAvatarUrl[] = "userImage";
const char kKeyNeedsSignin[] = "needsSignin";

// JS API callback names.
const char kJsApiUserManagerInitialize[] = "userManagerInitialize";
const char kJsApiUserManagerAddUser[] = "addUser";
const char kJsApiUserManagerAuthLaunchUser[] = "authenticatedLaunchUser";
const char kJsApiUserManagerLaunchGuest[] = "launchGuest";
const char kJsApiUserManagerLaunchUser[] = "launchUser";
const char kJsApiUserManagerRemoveUser[] = "removeUser";

const size_t kAvatarIconSize = 180;

void HandleAndDoNothing(const base::ListValue* args) {
}

// This callback is run if the only profile has been deleted, and a new
// profile has been created to replace it.
void OpenNewWindowForProfile(
    chrome::HostDesktopType desktop_type,
    Profile* profile,
    Profile::CreateStatus status) {
  if (status != Profile::CREATE_STATUS_INITIALIZED)
    return;
  profiles::FindOrCreateNewWindowForProfile(
    profile,
    chrome::startup::IS_PROCESS_STARTUP,
    chrome::startup::IS_FIRST_RUN,
    desktop_type,
    false);
}

std::string GetAvatarImageAtIndex(
    size_t index, const ProfileInfoCache& info_cache) {
  bool is_gaia_picture =
      info_cache.IsUsingGAIAPictureOfProfileAtIndex(index) &&
      info_cache.GetGAIAPictureOfProfileAtIndex(index);

  gfx::Image icon = profiles::GetSizedAvatarIconWithBorder(
      info_cache.GetAvatarIconOfProfileAtIndex(index),
      is_gaia_picture, kAvatarIconSize, kAvatarIconSize);
  return webui::GetBitmapDataUrl(icon.AsBitmap());
}

size_t GetIndexOfProfileWithEmailAndName(const ProfileInfoCache& info_cache,
                                         const string16& email,
                                         const string16& name) {
  for (size_t i = 0; i < info_cache.GetNumberOfProfiles(); ++i) {
    if (info_cache.GetUserNameOfProfileAtIndex(i) == email &&
        info_cache.GetNameOfProfileAtIndex(i) == name) {
      return i;
    }
  }
  return std::string::npos;
}

} // namespace

// ProfileUpdateObserver ------------------------------------------------------

class UserManagerScreenHandler::ProfileUpdateObserver
    : public ProfileInfoCacheObserver {
 public:
  ProfileUpdateObserver(
      ProfileManager* profile_manager, UserManagerScreenHandler* handler)
      : profile_manager_(profile_manager),
        user_manager_handler_(handler) {
    DCHECK(profile_manager_);
    DCHECK(user_manager_handler_);
    profile_manager_->GetProfileInfoCache().AddObserver(this);
  }

  virtual ~ProfileUpdateObserver() {
    DCHECK(profile_manager_);
    profile_manager_->GetProfileInfoCache().RemoveObserver(this);
  }

 private:
  // ProfileInfoCacheObserver implementation:
  // If any change has been made to a profile, propagate it to all the
  // visible user manager screens.
  virtual void OnProfileAdded(const base::FilePath& profile_path) OVERRIDE {
    user_manager_handler_->SendUserList();
  }

  virtual void OnProfileWasRemoved(
      const base::FilePath& profile_path,
      const base::string16& profile_name) OVERRIDE {
    // TODO(noms): Change 'SendUserList' to 'removeUser' JS-call when
    // UserManager is able to find pod belonging to removed user.
    user_manager_handler_->SendUserList();
  }

  virtual void OnProfileWillBeRemoved(
      const base::FilePath& profile_path) OVERRIDE {
    // No-op. When the profile is actually removed, OnProfileWasRemoved
    // will be called.
  }

  virtual void OnProfileNameChanged(
      const base::FilePath& profile_path,
      const base::string16& old_profile_name) OVERRIDE {
    user_manager_handler_->SendUserList();
  }

  virtual void OnProfileAvatarChanged(
      const base::FilePath& profile_path) OVERRIDE {
    user_manager_handler_->SendUserList();
  }

  ProfileManager* profile_manager_;

  UserManagerScreenHandler* user_manager_handler_;  // Weak; owns us.

  DISALLOW_COPY_AND_ASSIGN(ProfileUpdateObserver);
};

// UserManagerScreenHandler ---------------------------------------------------

UserManagerScreenHandler::UserManagerScreenHandler()
    : desktop_type_(chrome::GetActiveDesktop()) {
  profileInfoCacheObserver_.reset(
      new UserManagerScreenHandler::ProfileUpdateObserver(
          g_browser_process->profile_manager(), this));
}

UserManagerScreenHandler::~UserManagerScreenHandler() {
}

void UserManagerScreenHandler::HandleInitialize(const base::ListValue* args) {
  SendUserList();
  web_ui()->CallJavascriptFunction("cr.ui.Oobe.showUserManagerScreen");
  desktop_type_ = chrome::GetHostDesktopTypeForNativeView(
      web_ui()->GetWebContents()->GetView()->GetNativeView());
}

void UserManagerScreenHandler::HandleAddUser(const base::ListValue* args) {
  profiles::CreateAndSwitchToNewProfile(desktop_type_,
                                        base::Bind(&chrome::HideUserManager));
}

void UserManagerScreenHandler::HandleAuthenticatedLaunchUser(
    const base::ListValue* args) {
  string16 email_address;
  if (!args->GetString(0, &email_address))
    return;

  string16 display_name;
  if (!args->GetString(1, &display_name))
    return;

  std::string password;
  if (!args->GetString(2, &password))
    return;

  ProfileInfoCache& info_cache =
      g_browser_process->profile_manager()->GetProfileInfoCache();
  size_t profile_index = GetIndexOfProfileWithEmailAndName(
      info_cache, email_address, display_name);
  if (profile_index >= info_cache.GetNumberOfProfiles()) {
    NOTREACHED();
    return;
  }
  if (!chrome::ValidateLocalAuthCredentials(profile_index, password)) {
    web_ui()->CallJavascriptFunction(
        "cr.ui.Oobe.showSignInError",
        base::FundamentalValue(0),
        base::StringValue(
            l10n_util::GetStringUTF8(IDS_LOGIN_ERROR_AUTHENTICATING)),
        base::StringValue(""),
        base::FundamentalValue(0));
    return;
  }

  info_cache.SetProfileSigninRequiredAtIndex(profile_index, false);
  base::FilePath path = info_cache.GetPathOfProfileAtIndex(profile_index);
  profiles::SwitchToProfile(path, desktop_type_, true,
                            base::Bind(&chrome::HideUserManager));
}

void UserManagerScreenHandler::HandleRemoveUser(const base::ListValue* args) {
  DCHECK(args);
  const Value* profile_path_value;
  if (!args->Get(0, &profile_path_value))
    return;

  base::FilePath profile_path;
  if (!base::GetValueAsFilePath(*profile_path_value, &profile_path))
    return;

  // This handler could have been called in managed mode, for example because
  // the user fiddled with the web inspector. Silently return in this case.
  if (Profile::FromWebUI(web_ui())->IsManaged())
    return;

  if (!profiles::IsMultipleProfilesEnabled())
    return;

  g_browser_process->profile_manager()->ScheduleProfileForDeletion(
      profile_path,
      base::Bind(&OpenNewWindowForProfile, desktop_type_));
}

void UserManagerScreenHandler::HandleLaunchGuest(const base::ListValue* args) {
  profiles::SwitchToGuestProfile(desktop_type_,
                                 base::Bind(&chrome::HideUserManager));
}

void UserManagerScreenHandler::HandleLaunchUser(const base::ListValue* args) {
  base::string16 emailAddress;
  base::string16 displayName;

  if (!args->GetString(0, &emailAddress) ||
      !args->GetString(1, &displayName)) {
    NOTREACHED();
    return;
  }

  ProfileInfoCache& info_cache =
      g_browser_process->profile_manager()->GetProfileInfoCache();
  size_t profile_index = GetIndexOfProfileWithEmailAndName(
      info_cache, emailAddress, displayName);

  if (profile_index >= info_cache.GetNumberOfProfiles()) {
    NOTREACHED();
    return;
  }

  // It's possible that a user breaks into the user-manager page using the
  // JavaScript Inspector and causes a "locked" profile to call this
  // unauthenticated version of "launch" instead of the proper one.  Thus,
  // we have to validate in (secure) C++ code that it really is a profile
  // not needing authentication.  If it is, just ignore the "launch" request.
  if (info_cache.ProfileIsSigninRequiredAtIndex(profile_index))
    return;

  base::FilePath path = info_cache.GetPathOfProfileAtIndex(profile_index);
  profiles::SwitchToProfile(
      path, desktop_type_, true, base::Bind(&chrome::HideUserManager));
}

void UserManagerScreenHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(kJsApiUserManagerInitialize,
      base::Bind(&UserManagerScreenHandler::HandleInitialize,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback(kJsApiUserManagerAddUser,
      base::Bind(&UserManagerScreenHandler::HandleAddUser,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback(kJsApiUserManagerAuthLaunchUser,
      base::Bind(&UserManagerScreenHandler::HandleAuthenticatedLaunchUser,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback(kJsApiUserManagerLaunchGuest,
      base::Bind(&UserManagerScreenHandler::HandleLaunchGuest,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback(kJsApiUserManagerLaunchUser,
      base::Bind(&UserManagerScreenHandler::HandleLaunchUser,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback(kJsApiUserManagerRemoveUser,
      base::Bind(&UserManagerScreenHandler::HandleRemoveUser,
                 base::Unretained(this)));

  const content::WebUI::MessageCallback& kDoNothingCallback =
      base::Bind(&HandleAndDoNothing);

  // Unused callbacks from screen_account_picker.js
  web_ui()->RegisterMessageCallback("accountPickerReady", kDoNothingCallback);
  web_ui()->RegisterMessageCallback("loginUIStateChanged", kDoNothingCallback);
  web_ui()->RegisterMessageCallback("hideCaptivePortal", kDoNothingCallback);
  // Unused callbacks from display_manager.js
  web_ui()->RegisterMessageCallback("showAddUser", kDoNothingCallback);
  web_ui()->RegisterMessageCallback("loadWallpaper", kDoNothingCallback);
  web_ui()->RegisterMessageCallback("updateCurrentScreen", kDoNothingCallback);
  web_ui()->RegisterMessageCallback("loginVisible", kDoNothingCallback);
  // Unused callbacks from user_pod_row.js
  web_ui()->RegisterMessageCallback("focusPod", kDoNothingCallback);
}

void UserManagerScreenHandler::GetLocalizedValues(
    base::DictionaryValue* localized_strings) {
  // For Control Bar.
  localized_strings->SetString("signedIn",
      l10n_util::GetStringUTF16(IDS_SCREEN_LOCK_ACTIVE_USER));
  localized_strings->SetString("signinButton",
      l10n_util::GetStringUTF16(IDS_LOGIN_BUTTON));
  localized_strings->SetString("addUser",
      l10n_util::GetStringUTF16(IDS_ADD_USER_BUTTON));
  localized_strings->SetString("cancel", l10n_util::GetStringUTF16(IDS_CANCEL));
  localized_strings->SetString("browseAsGuest",
      l10n_util::GetStringUTF16(IDS_GO_INCOGNITO_BUTTON));
  localized_strings->SetString("signOutUser",
      l10n_util::GetStringUTF16(IDS_SCREEN_LOCK_SIGN_OUT));

  // For AccountPickerScreen.
  localized_strings->SetString("screenType", "login-add-user");
  localized_strings->SetString("highlightStrength", "normal");
  localized_strings->SetString("title",
      l10n_util::GetStringUTF16(IDS_USER_MANAGER_SCREEN_TITLE));
  localized_strings->SetString("passwordHint",
      l10n_util::GetStringUTF16(IDS_LOGIN_POD_EMPTY_PASSWORD_TEXT));
  localized_strings->SetString("podMenuButtonAccessibleName",
      l10n_util::GetStringUTF16(IDS_LOGIN_POD_MENU_BUTTON_ACCESSIBLE_NAME));
  localized_strings->SetString("podMenuRemoveItemAccessibleName",
      l10n_util::GetStringUTF16(
          IDS_LOGIN_POD_MENU_REMOVE_ITEM_ACCESSIBLE_NAME));
  localized_strings->SetString("removeUser",
      l10n_util::GetStringUTF16(IDS_LOGIN_POD_USER_REMOVE_WARNING_BUTTON));
  localized_strings->SetString("passwordFieldAccessibleName",
      l10n_util::GetStringUTF16(IDS_LOGIN_POD_PASSWORD_FIELD_ACCESSIBLE_NAME));
  localized_strings->SetString("bootIntoWallpaper", "off");

  // For AccountPickerScreen, the remove user warning overlay.
  localized_strings->SetString("removeUserWarningButtonTitle",
      l10n_util::GetStringUTF16(IDS_LOGIN_POD_USER_REMOVE_WARNING_BUTTON));
  localized_strings->SetString("removeUserWarningText",
      l10n_util::GetStringUTF16(
           IDS_LOGIN_POD_USER_REMOVE_WARNING));

  // Strings needed for the user_pod_template public account div, but not ever
  // actually displayed for desktop users.
  localized_strings->SetString("publicAccountReminder", base::string16());
  localized_strings->SetString("publicAccountEnter", base::string16());
  localized_strings->SetString("publicAccountEnterAccessibleName",
                               base::string16());
  localized_strings->SetString("multiple-signin-banner-text",
                               base::string16());
 }

void UserManagerScreenHandler::SendUserList() {
  ListValue users_list;
  base::FilePath active_profile_path =
      web_ui()->GetWebContents()->GetBrowserContext()->GetPath();
  const ProfileInfoCache& info_cache =
      g_browser_process->profile_manager()->GetProfileInfoCache();

  // If the active user is a managed user, then they may not perform
  // certain actions (i.e. delete another user).
  bool active_user_is_managed = Profile::FromWebUI(web_ui())->IsManaged();
  for (size_t i = 0; i < info_cache.GetNumberOfProfiles(); ++i) {
    DictionaryValue* profile_value = new DictionaryValue();

    base::FilePath profile_path = info_cache.GetPathOfProfileAtIndex(i);
    bool is_active_user = (profile_path == active_profile_path);

    profile_value->SetString(
        kKeyUsername, info_cache.GetUserNameOfProfileAtIndex(i));
    profile_value->SetString(
        kKeyEmailAddress, info_cache.GetUserNameOfProfileAtIndex(i));
    profile_value->SetString(
        kKeyDisplayName, info_cache.GetNameOfProfileAtIndex(i));
    profile_value->SetString(kKeyProfilePath, profile_path.MaybeAsASCII());
    profile_value->SetBoolean(kKeyPublicAccount, false);
    profile_value->SetBoolean(kKeyLocallyManagedUser, false);
    profile_value->SetBoolean(kKeySignedIn, is_active_user);
    profile_value->SetBoolean(
        kKeyNeedsSignin, info_cache.ProfileIsSigninRequiredAtIndex(i));
    profile_value->SetBoolean(kKeyIsOwner, false);
    profile_value->SetBoolean(kKeyCanRemove, !active_user_is_managed);
    profile_value->SetBoolean(kKeyIsDesktop, true);
    profile_value->SetString(
        kKeyAvatarUrl, GetAvatarImageAtIndex(i, info_cache));

    // The row of user pods should display the active user first.
    if (is_active_user)
      users_list.Insert(0, profile_value);
    else
      users_list.Append(profile_value);
  }

  web_ui()->CallJavascriptFunction("login.AccountPickerScreen.loadUsers",
    users_list, base::FundamentalValue(false), base::FundamentalValue(true));
}
