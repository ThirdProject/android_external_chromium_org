// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/cocoa/preferences_window_controller.h"

#include "base/mac_util.h"
#include "base/string_util.h"
#include "base/sys_string_conversions.h"
#include "chrome/browser/metrics/user_metrics.h"
#include "chrome/browser/net/url_fixer_upper.h"
#include "chrome/browser/profile.h"
#include "chrome/browser/session_startup_pref.h"
#include "chrome/browser/shell_integration.h"
#include "chrome/common/notification_details.h"
#include "chrome/common/notification_observer.h"
#include "chrome/common/notification_type.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/pref_service.h"
#include "chrome/common/url_constants.h"

NSString* const kUserDoneEditingPrefsNotification =
    @"kUserDoneEditingPrefsNotification";

namespace {
std::wstring GetNewTabUIURLString() {
  return UTF8ToWide(chrome::kChromeUINewTabURL);
}
}  // namespace

// A data source object for the "startup urls" table.
// TODO(pinkerton): hook this up to bindings.
@interface StartupURLDataSource : NSObject {
 @private
  Profile* profile_;  // weak, used to load icons
}
- (id)initWithProfile:(Profile*)profile;
@end

@implementation StartupURLDataSource
- (id)initWithProfile:(Profile*)profile {
  if ((self = [super init])) {
    profile_ = profile;
  }
  return self;
}
@end

//-------------------------------------------------------------------------

@interface PreferencesWindowController(Private)
// Callback when preferences are changed. |prefName| is the name of the
// pref that has changed, or |NULL| if all prefs should be updated.
- (void)prefChanged:(std::wstring*)prefName;
// Record the user performed a certain action and save the preferences.
- (void)recordUserAction:(const wchar_t*)action;
- (void)registerPrefObservers;
- (void)unregisterPrefObservers;

// KVC setter methods.
- (void)setNewTabPageIsHomePage:(NSInteger)val;
- (void)setHomepageURL:(NSString*)urlString;
- (void)setRestoreOnStartupIndex:(NSInteger)type;
- (void)setShowHomeButton:(BOOL)value;
- (void)setShowPageOptionsButtons:(BOOL)value;
- (void)setDefaultBrowser:(BOOL)value;
@end

// A C++ class registered for changes in preferences. Bridges the
// notification back to the PWC.
class PrefObserverBridge : public NotificationObserver {
 public:
  PrefObserverBridge(PreferencesWindowController* controller)
      : controller_(controller) { }
  // Overridden from NotificationObserver:
  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details) {
    if (type == NotificationType::PREF_CHANGED)
      [controller_ prefChanged:Details<std::wstring>(details).ptr()];
  }
 private:
  PreferencesWindowController* controller_;  // weak, owns us
};

@implementation PreferencesWindowController

- (id)initWithProfile:(Profile*)profile {
  DCHECK(profile);
  // Use initWithWindowNibPath:: instead of initWithWindowNibName: so we
  // can override it in a unit test.
  NSString *nibpath = [mac_util::MainAppBundle()
                        pathForResource:@"Preferences"
                                 ofType:@"nib"];
  if ((self = [super initWithWindowNibPath:nibpath owner:self])) {
    profile_ = profile;
    prefs_ = profile->GetPrefs();
    DCHECK(prefs_);
    observer_.reset(new PrefObserverBridge(self));
    customPagesSource_.reset([[StartupURLDataSource alloc]
                                initWithProfile:profile_]);
    // This needs to be done before awakeFromNib: because the bindings set up
    // in the nib rely on it.
    [self registerPrefObservers];
  }
  return self;
}

- (void)awakeFromNib {
  // TODO(pinkerton): save/restore size based on prefs.
  [[self window] center];

  // TODO(pinkerton): Ensure the "basics" tab is selected.
}

- (void)dealloc {
  [self unregisterPrefObservers];
  [super dealloc];
}

// Register our interest in the preferences we're displaying so if anything
// else in the UI changes them we will be updated.
- (void)registerPrefObservers {
  if (!prefs_) return;

  // Basics panel
  prefs_->AddPrefObserver(prefs::kURLsToRestoreOnStartup, observer_.get());
  restoreOnStartup_.Init(prefs::kRestoreOnStartup, prefs_, observer_.get());
  newTabPageIsHomePage_.Init(prefs::kHomePageIsNewTabPage,
                             prefs_, observer_.get());
  homepage_.Init(prefs::kHomePage, prefs_, observer_.get());
  showHomeButton_.Init(prefs::kShowHomeButton, prefs_, observer_.get());
  showPageOptionButtons_.Init(prefs::kShowPageOptionsButtons, prefs_,
                              observer_.get());
  // TODO(pinkerton): Register Default search.

  // TODO(pinkerton): do other panels...
}

// Clean up what was registered in -registerPrefObservers. We only have to
// clean up the non-PrefMember registrations.
- (void)unregisterPrefObservers {
  if (!prefs_) return;

  // Basics
  prefs_->RemovePrefObserver(prefs::kURLsToRestoreOnStartup, observer_.get());

  // TODO(pinkerton): do other panels...
}

// Record the user performed a certain action and save the preferences.
- (void)recordUserAction:(const wchar_t*)action {
  UserMetrics::RecordComputedAction(action, profile_);
  if (prefs_)
    prefs_->ScheduleSavePersistentPrefs();
}

// Returns the set of keys that |key| depends on for its value so it can be
// re-computed when any of those change as well.
+ (NSSet*)keyPathsForValuesAffectingValueForKey:(NSString*)key {
  NSSet* paths = [super keyPathsForValuesAffectingValueForKey:key];
  if ([key isEqualToString:@"isHomepageURLEnabled"]) {
    paths = [paths setByAddingObject:@"newTabPageIsHomePageIndex"];
  } else if ([key isEqualToString:@"enableRestoreButtons"]) {
    paths = [paths setByAddingObject:@"restoreOnStartupIndex"];
  } else if ([key isEqualToString:@"isDefaultBrowser"]) {
    paths = [paths setByAddingObject:@"defaultBrowser"];
  }
  return paths;
}

// Called when the user clicks the button to make Chromium the default
// browser. Registers http and https.
- (IBAction)makeDefaultBrowser:(id)sender {
  ShellIntegration::SetAsDefaultBrowser();

  // Tickle KVO so that the UI updates.
  [self setDefaultBrowser:YES];
}

// A stub setter so that we can trick KVO into thinking the UI needs
// to be updated.
- (void)setDefaultBrowser:(BOOL)ignore {
  // Do nothing.
}

// Returns if Chromium is the default browser.
- (BOOL)isDefaultBrowser {
  return ShellIntegration::IsDefaultBrowser() ? YES : NO;
}

//-------------------------------------------------------------------------
// Basics panel

// Sets the home page preferences for kNewTabPageIsHomePage and kHomePage. If a
// blank string is passed in we revert to using NewTab page as the Home page.
// When setting the Home Page to NewTab page, we preserve the old value of
// kHomePage (we don't overwrite it). Note: using SetValue() causes the
// observers not to fire, which is actually a good thing as we could end up in a
// state where setting the homepage to an empty url would automatically reset
// the prefs back to using the NTP, so we'd be never be able to change it.
- (void)setHomepage:(const std::wstring&)homepage {
  if (homepage.empty() || homepage == GetNewTabUIURLString()) {
    newTabPageIsHomePage_.SetValue(true);
  } else {
    newTabPageIsHomePage_.SetValue(false);
    homepage_.SetValue(homepage);
  }
}

// Callback when preferences are changed by someone modifying the prefs backend
// externally. |prefName| is the name of the pref that has changed. Unlike on
// Windows, we don't need to use this method for initializing, that's handled by
// Cocoa Bindings.
// Handles prefs for the "Basics" panel.
- (void)basicsPrefChanged:(std::wstring*)prefName {
  if (*prefName == prefs::kRestoreOnStartup) {
    const SessionStartupPref startupPref =
        SessionStartupPref::GetStartupPref(prefs_);
    [self setRestoreOnStartupIndex:startupPref.type];
  }

  // TODO(beng): Note that the kURLsToRestoreOnStartup pref is a mutable list,
  //             and changes to mutable lists aren't broadcast through the
  //             observer system, so the second half of this condition will
  //             never match. Once support for broadcasting such updates is
  //             added, this will automagically start to work, and this comment
  //             can be removed.
  if (*prefName == prefs::kURLsToRestoreOnStartup) {
    const SessionStartupPref startupPref =
        SessionStartupPref::GetStartupPref(prefs_);
    // Set table model.
    NOTIMPLEMENTED();
  }

  if (*prefName == prefs::kHomePageIsNewTabPage) {
    NSInteger useNewTabPage = newTabPageIsHomePage_.GetValue() ? 0 : 1;
    [self setNewTabPageIsHomePage:useNewTabPage];
  }
  if (*prefName == prefs::kHomePage) {
    NSString* value = base::SysWideToNSString(homepage_.GetValue());
    [self setHomepageURL:value];
  }

  if (*prefName == prefs::kShowHomeButton) {
    [self setShowHomeButton:showHomeButton_.GetValue() ? YES : NO];
  }
  if (*prefName == prefs::kShowPageOptionsButtons) {
    [self setShowPageOptionsButtons:showPageOptionButtons_.GetValue() ?
        YES : NO];
  }
}

// Returns the index of the selected cell in the "on startup" matrix based
// on the "restore on startup" pref. The ordering of the cells is in the
// same order as the pref.
- (NSInteger)restoreOnStartupIndex {
  const SessionStartupPref startupPref =
      SessionStartupPref::GetStartupPref(prefs_);
  return startupPref.type;
}

// Sets the pref based on the index of the selected cell in the matrix and
// marks the appropriate user metric.
- (void)setRestoreOnStartupIndex:(NSInteger)type {
  SessionStartupPref pref;
  pref.type = static_cast<SessionStartupPref::Type>(type);
  // TODO(pinkerton): list of pages in |pref.urls|
  switch (pref.type) {
    case SessionStartupPref::DEFAULT:
      [self recordUserAction:L"Options_Startup_Homepage"];
      break;
    case SessionStartupPref::LAST:
      [self recordUserAction:L"Options_Startup_LastSession"];
      break;
    case SessionStartupPref::URLS:
      [self recordUserAction:L"Options_Startup_Custom"];
      break;
    default:
      NOTREACHED();
  }
  SessionStartupPref::SetStartupPref(prefs_, pref);
}

// Returns whether or not the +/-/Current buttons should be enabled, based on
// the current pref value for the startup urls.
- (BOOL)enableRestoreButtons {
  return [self restoreOnStartupIndex] == SessionStartupPref::URLS;
}

enum { kHomepageNewTabPage, kHomepageURL };

// Returns the index of the selected cell in the "home page" marix based on
// the "new tab is home page" pref. Sadly, the ordering is reversed from the
// pref value.
- (NSInteger)newTabPageIsHomePageIndex {
  return newTabPageIsHomePage_.GetValue() ?
      kHomepageNewTabPage : kHomepageURL;
}

// Sets the pref based on the given index into the matrix and marks the
// appropriate user metric.
- (void)setNewTabPageIsHomePageIndex:(NSInteger)index {
  bool useNewTabPage = index == kHomepageNewTabPage ? true : false;
  if (useNewTabPage)
    [self recordUserAction:L"Options_Homepage_UseNewTab"];
  else
    [self recordUserAction:L"Options_Homepage_UseURL"];
  newTabPageIsHomePage_.SetValue(useNewTabPage);
}

// Returns whether or not the homepage URL text field should be enabled
// based on if the new tab page is the home page.
- (BOOL)isHomepageURLEnabled {
  return newTabPageIsHomePage_.GetValue() ? NO : YES;
}

// Returns the homepage URL.
- (NSString*)homepageURL {
  NSString* value = base::SysWideToNSString(homepage_.GetValue());
  return value;
}

// Sets the homepage URL to |urlString| with some fixing up.
- (void)setHomepageURL:(NSString*)urlString {
  // If the text field contains a valid URL, sync it to prefs. We run it
  // through the fixer upper to allow input like "google.com" to be converted
  // to something valid ("http://google.com").
  std::wstring temp = base::SysNSStringToWide(urlString);
  std::wstring fixedString = URLFixerUpper::FixupURL(temp, std::wstring());
  if (GURL(WideToUTF8(fixedString)).is_valid())
    [self setHomepage:fixedString];
}

// Returns whether the home button should be checked based on the preference.
- (BOOL)showHomeButton {
  return showHomeButton_.GetValue() ? YES : NO;
}

// Sets the backend pref for whether or not the home button should be displayed
// based on |value|.
- (void)setShowHomeButton:(BOOL)value {
  if (value)
    [self recordUserAction:L"Options_Homepage_ShowHomeButton"];
  else
    [self recordUserAction:L"Options_Homepage_HideHomeButton"];
  showHomeButton_.SetValue(value ? true : false);
}

// Returns whether the page and options button should be checked based on the
// preference.
- (BOOL)showPageOptionsButtons {
  return showPageOptionButtons_.GetValue() ? YES : NO;
}

// Sets the backend pref for whether or not the page and options buttons should
// be displayed based on |value|.
- (void)setShowPageOptionsButtons:(BOOL)value {
  if (value)
    [self recordUserAction:L"Options_Homepage_ShowPageOptionsButtons"];
  else
    [self recordUserAction:L"Options_Homepage_HidePageOptionsButtons"];
  showPageOptionButtons_.SetValue(value ? true : false);
}

//-------------------------------------------------------------------------
// Minor Tweaks panel

// Callback when preferences are changed. |prefName| is the name of the
// pref that has changed, or |NULL| if all prefs should be updated.
// Handles prefs for the "Minor Tweaks" panel.
- (void)minorTweaksPrefChanged:(std::wstring*)prefName {
}

//-------------------------------------------------------------------------
// Under the hood panel

// Callback when preferences are changed. |prefName| is the name of the
// pref that has changed, or |NULL| if all prefs should be updated.
// Handles prefs for the "Under the hood" panel.
- (void)underHoodPrefChanged:(std::wstring*)prefName {
}

//-------------------------------------------------------------------------

// Callback when preferences are changed. |prefName| is the name of the
// pref that has changed and should not be NULL.
- (void)prefChanged:(std::wstring*)prefName {
  DCHECK(prefName);
  if (!prefName) return;
  [self basicsPrefChanged:prefName];
  [self minorTweaksPrefChanged:prefName];
  [self underHoodPrefChanged:prefName];
}

// Show the preferences window.
- (IBAction)showPreferences:(id)sender {
  [self showWindow:sender];
}

// Called when the window is being closed. Send out a notification that the
// user is done editing preferences.
- (void)windowWillClose:(NSNotification *)notification {
  [[NSNotificationCenter defaultCenter]
      postNotificationName:kUserDoneEditingPrefsNotification
                    object:self];
}

@end
