// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/browser_action_test_util.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_test_message_listener.h"
#include "chrome/browser/extensions/user_script_master.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "net/base/mock_host_resolver.h"

using content::WebContents;

IN_PROC_BROWSER_TEST_F(ExtensionApiTest, IncognitoNoScript) {
  ASSERT_TRUE(StartTestServer());

  // Loads a simple extension which attempts to change the title of every page
  // that loads to "modified".
  ASSERT_TRUE(LoadExtension(test_data_dir_.AppendASCII("incognito")
      .AppendASCII("content_scripts")));

  // Open incognito window and navigate to test page.
  Browser* otr_browser = ui_test_utils::OpenURLOffTheRecord(
      browser()->profile(),
      test_server()->GetURL("files/extensions/test_file.html"));

  WebContents* tab = chrome::GetActiveWebContents(otr_browser);

  // Verify the script didn't run.
  bool result = false;
  ASSERT_TRUE(content::ExecuteJavaScriptAndExtractBool(
      tab->GetRenderViewHost(),
      "",
      "window.domAutomationController.send(document.title == 'Unmodified')",
      &result));
  EXPECT_TRUE(result);
}

IN_PROC_BROWSER_TEST_F(ExtensionApiTest, IncognitoYesScript) {
  host_resolver()->AddRule("*", "127.0.0.1");
  ASSERT_TRUE(StartTestServer());

  // Load a dummy extension. This just tests that we don't regress a
  // crash fix when multiple incognito- and non-incognito-enabled extensions
  // are mixed.
  ASSERT_TRUE(LoadExtension(test_data_dir_.AppendASCII("content_scripts")
      .AppendASCII("all_frames")));

  // Loads a simple extension which attempts to change the title of every page
  // that loads to "modified".
  ASSERT_TRUE(LoadExtensionIncognito(test_data_dir_
      .AppendASCII("incognito").AppendASCII("content_scripts")));

  // Dummy extension #2.
  ASSERT_TRUE(LoadExtension(test_data_dir_
      .AppendASCII("content_scripts").AppendASCII("isolated_world1")));

  // Open incognito window and navigate to test page.
  Browser* otr_browser = ui_test_utils::OpenURLOffTheRecord(
      browser()->profile(),
      test_server()->GetURL("files/extensions/test_file.html"));

  WebContents* tab = chrome::GetActiveWebContents(otr_browser);

  // Verify the script ran.
  bool result = false;
  ASSERT_TRUE(content::ExecuteJavaScriptAndExtractBool(
      tab->GetRenderViewHost(),
      "",
      "window.domAutomationController.send(document.title == 'modified')",
      &result));
  EXPECT_TRUE(result);
}

// Tests that an extension which is enabled for incognito mode doesn't
// accidentially create and incognito profile.
// Test disabled due to http://crbug.com/89054.
IN_PROC_BROWSER_TEST_F(ExtensionApiTest, DISABLED_DontCreateIncognitoProfile) {
  ASSERT_FALSE(browser()->profile()->HasOffTheRecordProfile());
  ASSERT_TRUE(RunExtensionTestIncognito(
      "incognito/dont_create_profile")) << message_;
  ASSERT_FALSE(browser()->profile()->HasOffTheRecordProfile());
}

#if defined(OS_WIN) || defined(OS_MACOSX)
// http://crbug.com/120484
IN_PROC_BROWSER_TEST_F(ExtensionApiTest, DISABLED_Incognito) {
#else
IN_PROC_BROWSER_TEST_F(ExtensionApiTest, Incognito) {
#endif
  host_resolver()->AddRule("*", "127.0.0.1");
  ASSERT_TRUE(StartTestServer());

  ResultCatcher catcher;

  // Open incognito window and navigate to test page.
  ui_test_utils::OpenURLOffTheRecord(
      browser()->profile(),
      test_server()->GetURL("files/extensions/test_file.html"));

  ASSERT_TRUE(LoadExtensionIncognito(test_data_dir_
      .AppendASCII("incognito").AppendASCII("apis")));

  EXPECT_TRUE(catcher.GetNextResult()) << catcher.message();
}

// Tests that the APIs in an incognito-enabled split-mode extension work
// properly.
#if defined(OS_WIN)
// http://crbug.com/120484
IN_PROC_BROWSER_TEST_F(ExtensionApiTest, DISABLED_IncognitoSplitMode) {
#else
IN_PROC_BROWSER_TEST_F(ExtensionApiTest, IncognitoSplitMode) {
#endif
  host_resolver()->AddRule("*", "127.0.0.1");
  ASSERT_TRUE(StartTestServer());

  // We need 2 ResultCatchers because we'll be running the same test in both
  // regular and incognito mode.
  ResultCatcher catcher;
  catcher.RestrictToProfile(browser()->profile());
  ResultCatcher catcher_incognito;
  catcher_incognito.RestrictToProfile(
      browser()->profile()->GetOffTheRecordProfile());

  ExtensionTestMessageListener listener("waiting", true);
  ExtensionTestMessageListener listener_incognito("waiting_incognito", true);

  // Open incognito window and navigate to test page.
  ui_test_utils::OpenURLOffTheRecord(
      browser()->profile(),
      test_server()->GetURL("files/extensions/test_file.html"));

  ASSERT_TRUE(LoadExtensionIncognito(test_data_dir_
      .AppendASCII("incognito").AppendASCII("split")));

  // Wait for both extensions to be ready before telling them to proceed.
  EXPECT_TRUE(listener.WaitUntilSatisfied());
  EXPECT_TRUE(listener_incognito.WaitUntilSatisfied());
  listener.Reply("go");
  listener_incognito.Reply("go");

  EXPECT_TRUE(catcher.GetNextResult()) << catcher.message();
  EXPECT_TRUE(catcher_incognito.GetNextResult()) << catcher.message();
}

// Tests that the APIs in an incognito-disabled extension don't see incognito
// events or callbacks.
#if defined(OS_WIN)
// http://crbug.com/120484
IN_PROC_BROWSER_TEST_F(ExtensionApiTest, DISABLED_IncognitoDisabled) {
#else
IN_PROC_BROWSER_TEST_F(ExtensionApiTest, IncognitoDisabled) {
#endif
  host_resolver()->AddRule("*", "127.0.0.1");
  ASSERT_TRUE(StartTestServer());

  ResultCatcher catcher;

  // Open incognito window and navigate to test page.
  ui_test_utils::OpenURLOffTheRecord(
      browser()->profile(),
      test_server()->GetURL("files/extensions/test_file.html"));

  ASSERT_TRUE(LoadExtension(test_data_dir_
      .AppendASCII("incognito").AppendASCII("apis_disabled")));

  EXPECT_TRUE(catcher.GetNextResult()) << catcher.message();
}

#if defined(OS_WIN)
// http://crbug.com/104438.
#define MAYBE_IncognitoPopup DISABLED_IncognitoPopup
#else
#define MAYBE_IncognitoPopup IncognitoPopup
#endif
// Test that opening a popup from an incognito browser window works properly.
IN_PROC_BROWSER_TEST_F(ExtensionApiTest, MAYBE_IncognitoPopup) {
  host_resolver()->AddRule("*", "127.0.0.1");
  ASSERT_TRUE(StartTestServer());

  ResultCatcher catcher;

  ASSERT_TRUE(LoadExtensionIncognito(test_data_dir_
      .AppendASCII("incognito").AppendASCII("popup")));

  // Open incognito window and navigate to test page.
  Browser* incognito_browser = ui_test_utils::OpenURLOffTheRecord(
      browser()->profile(),
      test_server()->GetURL("files/extensions/test_file.html"));

  // Simulate the incognito's browser action being clicked.
  BrowserActionTestUtil(incognito_browser).Press(0);

  EXPECT_TRUE(catcher.GetNextResult()) << catcher.message();
}
