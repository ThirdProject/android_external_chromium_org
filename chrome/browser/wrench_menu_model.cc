// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/wrench_menu_model.h"

#include <algorithm>
#include <cmath>

#include "app/l10n_util.h"
#include "app/menus/button_menu_item_model.h"
#include "app/resource_bundle.h"
#include "base/command_line.h"
#include "base/string_number_conversions.h"
#include "base/string_util.h"
#include "base/utf_string_conversions.h"
#include "chrome/app/chrome_dll_resource.h"
#include "chrome/browser/browser.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/defaults.h"
#include "chrome/browser/encoding_menu_controller.h"
#include "chrome/browser/host_zoom_map.h"
#include "chrome/browser/pref_service.h"
#include "chrome/browser/profile.h"
#include "chrome/browser/sync/profile_sync_service.h"
#include "chrome/browser/sync/sync_ui_util.h"
#include "chrome/browser/tab_contents/tab_contents.h"
#include "chrome/browser/upgrade_detector.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/notification_service.h"
#include "chrome/common/notification_source.h"
#include "chrome/common/notification_type.h"
#include "chrome/common/pref_names.h"
#include "grit/chromium_strings.h"
#include "grit/generated_resources.h"
#include "grit/theme_resources.h"

#if defined(OS_LINUX)
#include <gtk/gtk.h>
#endif

#if defined(OS_MACOSX)
#include "chrome/browser/browser_window.h"
#endif

////////////////////////////////////////////////////////////////////////////////
// EncodingMenuModel

EncodingMenuModel::EncodingMenuModel(Browser* browser)
    : ALLOW_THIS_IN_INITIALIZER_LIST(menus::SimpleMenuModel(this)),
      browser_(browser) {
  Build();
}

void EncodingMenuModel::Build() {
  EncodingMenuController::EncodingMenuItemList encoding_menu_items;
  EncodingMenuController encoding_menu_controller;
  encoding_menu_controller.GetEncodingMenuItems(browser_->profile(),
                                                &encoding_menu_items);

  int group_id = 0;
  EncodingMenuController::EncodingMenuItemList::iterator it =
      encoding_menu_items.begin();
  for (; it != encoding_menu_items.end(); ++it) {
    int id = it->first;
    string16& label = it->second;
    if (id == 0) {
      AddSeparator();
    } else {
      if (id == IDC_ENCODING_AUTO_DETECT) {
        AddCheckItem(id, label);
      } else {
        // Use the id of the first radio command as the id of the group.
        if (group_id <= 0)
          group_id = id;
        AddRadioItem(id, label, group_id);
      }
    }
  }
}

bool EncodingMenuModel::IsCommandIdChecked(int command_id) const {
  TabContents* current_tab = browser_->GetSelectedTabContents();
  if (!current_tab)
    return false;
  EncodingMenuController controller;
  return controller.IsItemChecked(browser_->profile(),
                                  current_tab->encoding(), command_id);
}

bool EncodingMenuModel::IsCommandIdEnabled(int command_id) const {
  bool enabled = browser_->command_updater()->IsCommandEnabled(command_id);
  // Special handling for the contents of the Encoding submenu. On Mac OS,
  // instead of enabling/disabling the top-level menu item, the submenu's
  // contents get disabled, per Apple's HIG.
#if defined(OS_MACOSX)
  enabled &= browser_->command_updater()->IsCommandEnabled(IDC_ENCODING_MENU);
#endif
  return enabled;
}

bool EncodingMenuModel::GetAcceleratorForCommandId(
    int command_id,
    menus::Accelerator* accelerator) {
  return false;
}

void EncodingMenuModel::ExecuteCommand(int command_id) {
  browser_->ExecuteCommand(command_id);
}

////////////////////////////////////////////////////////////////////////////////
// ZoomMenuModel

ZoomMenuModel::ZoomMenuModel(menus::SimpleMenuModel::Delegate* delegate)
    : SimpleMenuModel(delegate) {
  Build();
}

void ZoomMenuModel::Build() {
  AddItemWithStringId(IDC_ZOOM_PLUS, IDS_ZOOM_PLUS);
  AddItemWithStringId(IDC_ZOOM_NORMAL, IDS_ZOOM_NORMAL);
  AddItemWithStringId(IDC_ZOOM_MINUS, IDS_ZOOM_MINUS);
}

////////////////////////////////////////////////////////////////////////////////
// ToolsMenuModel

ToolsMenuModel::ToolsMenuModel(menus::SimpleMenuModel::Delegate* delegate,
                               Browser* browser)
    : SimpleMenuModel(delegate) {
  Build(browser);
}

ToolsMenuModel::~ToolsMenuModel() {}

void ToolsMenuModel::Build(Browser* browser) {
  AddCheckItemWithStringId(IDC_SHOW_BOOKMARK_BAR, IDS_SHOW_BOOKMARK_BAR);

  AddSeparator();

#if !defined(OS_CHROMEOS)
#if defined(OS_MACOSX)
  AddItemWithStringId(IDC_CREATE_SHORTCUTS, IDS_CREATE_APPLICATION_MAC);
#else
  AddItemWithStringId(IDC_CREATE_SHORTCUTS, IDS_CREATE_SHORTCUTS);
#endif
  AddSeparator();
#endif

  AddItemWithStringId(IDC_MANAGE_EXTENSIONS, IDS_SHOW_EXTENSIONS);
  AddItemWithStringId(IDC_TASK_MANAGER, IDS_TASK_MANAGER);
  AddItemWithStringId(IDC_CLEAR_BROWSING_DATA, IDS_CLEAR_BROWSING_DATA);

  AddSeparator();
  AddItemWithStringId(IDC_REPORT_BUG, IDS_REPORT_BUG);
  AddSeparator();

  encoding_menu_model_.reset(new EncodingMenuModel(browser));
  AddSubMenuWithStringId(IDC_ENCODING_MENU, IDS_ENCODING_MENU,
                         encoding_menu_model_.get());
  AddItemWithStringId(IDC_VIEW_SOURCE, IDS_VIEW_SOURCE);
  if (g_browser_process->have_inspector_files()) {
    AddItemWithStringId(IDC_DEV_TOOLS, IDS_DEV_TOOLS);
    AddItemWithStringId(IDC_DEV_TOOLS_CONSOLE, IDS_DEV_TOOLS_CONSOLE);
  }
}

////////////////////////////////////////////////////////////////////////////////
// WrenchMenuModel

WrenchMenuModel::WrenchMenuModel(menus::AcceleratorProvider* provider,
                                 Browser* browser)
    : ALLOW_THIS_IN_INITIALIZER_LIST(model_(this)),
      provider_(provider),
      browser_(browser),
      tabstrip_model_(browser_->tabstrip_model()) {
  Build();
  UpdateZoomControls();

  tabstrip_model_->AddObserver(this);

  registrar_.Add(this, NotificationType::ZOOM_LEVEL_CHANGED,
                 Source<Profile>(browser_->profile()));
  registrar_.Add(this, NotificationType::NAV_ENTRY_COMMITTED,
                 NotificationService::AllSources());
}

WrenchMenuModel::~WrenchMenuModel() {
  if (tabstrip_model_)
    tabstrip_model_->RemoveObserver(this);
}

bool WrenchMenuModel::IsLabelForCommandIdDynamic(int command_id) const {
  return command_id == IDC_ZOOM_PERCENT_DISPLAY ||
         command_id == IDC_SYNC_BOOKMARKS ||
#if defined(OS_MACOSX)
         command_id == IDC_FULLSCREEN ||
#endif
         command_id == IDC_ABOUT;
}

string16 WrenchMenuModel::GetLabelForCommandId(int command_id) const {
  switch (command_id) {
    case IDC_ABOUT:
      return GetAboutEntryMenuLabel();
    case IDC_SYNC_BOOKMARKS:
      return GetSyncMenuLabel();
    case IDC_ZOOM_PERCENT_DISPLAY:
      return zoom_label_;
#if defined(OS_MACOSX)
    case IDC_FULLSCREEN: {
      int string_id = IDS_ENTER_FULLSCREEN_MAC;  // Default to Enter.
      // Note: On startup, |window()| may be NULL.
      if (browser_->window() && browser_->window()->IsFullscreen())
        string_id = IDS_EXIT_FULLSCREEN_MAC;
      return l10n_util::GetStringUTF16(string_id);
    }
#endif
    default:
      NOTREACHED();
      return string16();
  }
}

void WrenchMenuModel::ExecuteCommand(int command_id) {
  browser_->ExecuteCommand(command_id);
}

bool WrenchMenuModel::IsCommandIdChecked(int command_id) const {
#if defined(OS_CHROMEOS)
  if (command_id == IDC_TOGGLE_VERTICAL_TABS) {
    return browser_->UseVerticalTabs();
  }
#endif

  if (command_id == IDC_SHOW_BOOKMARK_BAR) {
    return browser_->profile()->GetPrefs()->GetBoolean(prefs::kShowBookmarkBar);
  }

  return false;
}

bool WrenchMenuModel::IsCommandIdEnabled(int command_id) const {
  return browser_->command_updater()->IsCommandEnabled(command_id);
}

bool WrenchMenuModel::GetAcceleratorForCommandId(
      int command_id,
      menus::Accelerator* accelerator) {
  return provider_->GetAcceleratorForCommandId(command_id, accelerator);
}

void WrenchMenuModel::TabSelectedAt(TabContents* old_contents,
                                    TabContents* new_contents,
                                    int index,
                                    bool user_gesture) {
  // The user has switched between tabs and the new tab may have a different
  // zoom setting.
  UpdateZoomControls();
}

void WrenchMenuModel::TabReplacedAt(TabContents* old_contents,
                                    TabContents* new_contents, int index) {
  UpdateZoomControls();
}

void WrenchMenuModel::TabStripModelDeleted() {
  // During views shutdown, the tabstrip model/browser is deleted first, while
  // it is the opposite in gtk land.
  tabstrip_model_->RemoveObserver(this);
  tabstrip_model_ = NULL;
}

void WrenchMenuModel::Observe(NotificationType type,
                              const NotificationSource& source,
                              const NotificationDetails& details) {
  UpdateZoomControls();
}

// For testing.
WrenchMenuModel::WrenchMenuModel() : model_(NULL) {
}

void WrenchMenuModel::Build() {
  model_.AddItemWithStringId(IDC_NEW_TAB, IDS_NEW_TAB);
  model_.AddItemWithStringId(IDC_NEW_WINDOW, IDS_NEW_WINDOW);
  model_.AddItemWithStringId(IDC_NEW_INCOGNITO_WINDOW,
                             IDS_NEW_INCOGNITO_WINDOW);

  model_.AddSeparator();
#if defined(OS_MACOSX) || (defined(OS_LINUX) && !defined(TOOLKIT_VIEWS))
  // WARNING: Mac does not use the ButtonMenuItemModel, but instead defines the
  // layout for this menu item in Toolbar.xib. It does, however, use the
  // command_id value from AddButtonItem() to identify this special item.
  edit_menu_item_model_.reset(new menus::ButtonMenuItemModel(IDS_EDIT, this));
  edit_menu_item_model_->AddGroupItemWithStringId(IDC_CUT, IDS_CUT);
  edit_menu_item_model_->AddGroupItemWithStringId(IDC_COPY, IDS_COPY);
  edit_menu_item_model_->AddGroupItemWithStringId(IDC_PASTE, IDS_PASTE);
  model_.AddButtonItem(IDC_EDIT_MENU, edit_menu_item_model_.get());
#else
  // TODO(port): Move to the above.
  CreateCutCopyPaste();
#endif

  model_.AddSeparator();
#if defined(OS_MACOSX) || (defined(OS_LINUX) && !defined(TOOLKIT_VIEWS))
  // WARNING: See above comment.
  zoom_menu_item_model_.reset(
      new menus::ButtonMenuItemModel(IDS_ZOOM_MENU, this));
  zoom_menu_item_model_->AddGroupItemWithStringId(
      IDC_ZOOM_MINUS, IDS_ZOOM_MINUS2);
  zoom_menu_item_model_->AddButtonLabel(IDC_ZOOM_PERCENT_DISPLAY,
                                        IDS_ZOOM_PLUS2);
  zoom_menu_item_model_->AddGroupItemWithStringId(
      IDC_ZOOM_PLUS, IDS_ZOOM_PLUS2);
  zoom_menu_item_model_->AddSpace();
  zoom_menu_item_model_->AddItemWithImage(
      IDC_FULLSCREEN, IDR_FULLSCREEN_MENU_BUTTON);
  model_.AddButtonItem(IDC_ZOOM_MENU, zoom_menu_item_model_.get());
#else
  // TODO(port): Move to the above.
  CreateZoomFullscreen();
#endif

  model_.AddSeparator();
  model_.AddItemWithStringId(IDC_SAVE_PAGE, IDS_SAVE_PAGE);
  model_.AddItemWithStringId(IDC_FIND, IDS_FIND);
  model_.AddItemWithStringId(IDC_PRINT, IDS_PRINT);

  tools_menu_model_.reset(new ToolsMenuModel(this, browser_));
  model_.AddSubMenuWithStringId(IDC_ZOOM_MENU, IDS_TOOLS_MENU,
                         tools_menu_model_.get());

  model_.AddSeparator();
#if defined(ENABLE_REMOTING)
  if (CommandLine::ForCurrentProcess()->HasSwitch(switches::kEnableRemoting)) {
    model_.AddItem(IDC_REMOTING_SETUP,
            l10n_util::GetStringUTF16(IDS_REMOTING_SETUP_LABEL));
  }
#endif
  model_.AddItemWithStringId(IDC_SHOW_BOOKMARK_MANAGER, IDS_BOOKMARK_MANAGER);
  model_.AddItemWithStringId(IDC_SHOW_HISTORY, IDS_SHOW_HISTORY);
  model_.AddItemWithStringId(IDC_SHOW_DOWNLOADS, IDS_SHOW_DOWNLOADS);
  model_.AddSeparator();

#if defined(OS_MACOSX)
  model_.AddItemWithStringId(IDC_OPTIONS, IDS_PREFERENCES_MAC);
#elif defined(OS_LINUX)
  GtkStockItem stock_item;
  if (gtk_stock_lookup(GTK_STOCK_PREFERENCES, &stock_item)) {
    const char16 kUnderscore[] = { '_', 0 };
    string16 preferences;
    RemoveChars(UTF8ToUTF16(stock_item.label), kUnderscore, &preferences);
    model_.AddItem(IDC_OPTIONS, preferences);
  } else {
    model_.AddItemWithStringId(IDC_OPTIONS, IDS_OPTIONS);
  }
#else
  model_.AddItemWithStringId(IDC_OPTIONS, IDS_OPTIONS);
#endif

#if defined(OS_CHROMEOS)
  model_.AddCheckItemWithStringId(IDC_TOGGLE_VERTICAL_TABS,
                           IDS_TAB_CXMENU_USE_VERTICAL_TABS);
#endif

  // TODO(erg): This entire section needs to be reworked and is out of scope of
  // the first cleanup patch I'm doing. Part 1 (crbug.com/47320) is moving most
  // of the logic from each platform specific UI code into the model here. Part
  // 2 (crbug.com/46221) is normalizing the about menu item/hidden update menu
  // item behaviour across the three platforms.

  // On Mac, there is no About item unless it is replaced with the update
  // available notification.
  if (browser_defaults::kShowAboutMenuItem ||
      Singleton<UpgradeDetector>::get()->notify_upgrade()) {
    model_.AddItem(IDC_ABOUT,
            l10n_util::GetStringFUTF16(
                IDS_ABOUT,
                l10n_util::GetStringUTF16(IDS_PRODUCT_NAME)));
    // ResourceBundle& rb = ResourceBundle::GetSharedInstance();
    // model_.SetIcon(model_.GetIndexOfCommandId(IDC_ABOUT),
    //                *rb.GetBitmapNamed(IDR_UPDATE_AVAILABLE));
  }
  model_.AddItemWithStringId(IDC_HELP_PAGE, IDS_HELP_PAGE);
  if (browser_defaults::kShowExitMenuItem) {
    model_.AddSeparator();
#if defined(OS_CHROMEOS)
    model_.AddItemWithStringId(IDC_EXIT, IDS_SIGN_OUT);
#else
    model_.AddItemWithStringId(IDC_EXIT, IDS_EXIT);
#endif
  }
}

void WrenchMenuModel::CreateCutCopyPaste() {
  // WARNING: views/wrench_menu assumes these items are added in this order. If
  // you change the order you'll need to update wrench_menu as well.
  model_.AddItemWithStringId(IDC_CUT, IDS_CUT);
  model_.AddItemWithStringId(IDC_COPY, IDS_COPY);
  model_.AddItemWithStringId(IDC_PASTE, IDS_PASTE);
}

void WrenchMenuModel::CreateZoomFullscreen() {
  // WARNING: views/wrench_menu assumes these items are added in this order. If
  // you change the order you'll need to update wrench_menu as well.
  model_.AddItemWithStringId(IDC_ZOOM_MINUS, IDS_ZOOM_MINUS);
  model_.AddItemWithStringId(IDC_ZOOM_PLUS, IDS_ZOOM_PLUS);
  model_.AddItemWithStringId(IDC_FULLSCREEN, IDS_FULLSCREEN);
}

void WrenchMenuModel::UpdateZoomControls() {
  bool enable_increment, enable_decrement;
  int zoom_percent =
      static_cast<int>(GetZoom(&enable_increment, &enable_decrement) * 100);
  zoom_label_ = l10n_util::GetStringFUTF16(
      IDS_ZOOM_PERCENT, base::IntToString16(zoom_percent));
}

double WrenchMenuModel::GetZoom(bool* enable_increment,
                                bool* enable_decrement) {
  TabContents* selected_tab = browser_->GetSelectedTabContents();
  *enable_decrement = *enable_increment = false;
  if (!selected_tab)
    return 1;

  HostZoomMap* zoom_map = selected_tab->profile()->GetHostZoomMap();
  if (!zoom_map)
    return 1;

  // This code comes from  WebViewImpl::setZoomLevel.
  int zoom_level = zoom_map->GetZoomLevel(selected_tab->GetURL());
  double value = static_cast<double>(
      std::max(std::min(std::pow(1.2, zoom_level), 3.0), .5));
  *enable_decrement = (value != .5);
  *enable_increment = (value != 3.0);
  return value;
}

string16 WrenchMenuModel::GetSyncMenuLabel() const {
  return sync_ui_util::GetSyncMenuLabel(
      browser_->profile()->GetOriginalProfile()->GetProfileSyncService());
}

string16 WrenchMenuModel::GetAboutEntryMenuLabel() const {
  if (Singleton<UpgradeDetector>::get()->notify_upgrade()) {
    return l10n_util::GetStringFUTF16(
        IDS_UPDATE_NOW, l10n_util::GetStringUTF16(IDS_PRODUCT_NAME));
  }
  return l10n_util::GetStringFUTF16(
      IDS_ABOUT, l10n_util::GetStringUTF16(IDS_PRODUCT_NAME));
}
