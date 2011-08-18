// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/BidiCheckerWebUITest.h"

#include "base/base_paths.h"
#include "base/path_service.h"
#include "base/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/autofill/autofill_common_test.h"
#include "chrome/browser/autofill/autofill_profile.h"
#include "chrome/browser/autofill/personal_data_manager.h"
#include "chrome/browser/history/history.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/ui_test_utils.h"
#include "ui/base/resource/resource_bundle.h"

static const FilePath::CharType* kWebUIBidiCheckerLibraryJS =
    FILE_PATH_LITERAL("third_party/bidichecker/bidichecker_packaged.js");

namespace {
FilePath WebUIBidiCheckerLibraryJSPath() {
  FilePath src_root;
  if (!PathService::Get(base::DIR_SOURCE_ROOT, &src_root))
    LOG(ERROR) << "Couldn't find source root";
  return src_root.Append(kWebUIBidiCheckerLibraryJS);
}
}

static const FilePath::CharType* kBidiCheckerTestsJS =
    FILE_PATH_LITERAL("bidichecker_tests.js");

WebUIBidiCheckerBrowserTest::~WebUIBidiCheckerBrowserTest() {}

WebUIBidiCheckerBrowserTest::WebUIBidiCheckerBrowserTest() {}

void WebUIBidiCheckerBrowserTest::SetUpInProcessBrowserTestFixture() {
  WebUIBrowserTest::SetUpInProcessBrowserTestFixture();
  WebUIBrowserTest::AddLibrary(WebUIBidiCheckerLibraryJSPath());
  WebUIBrowserTest::AddLibrary(FilePath(kBidiCheckerTestsJS));
}

void WebUIBidiCheckerBrowserTest::RunBidiCheckerOnPage(const char pageURL[],
                                                       bool isRTL) {
  ui_test_utils::NavigateToURL(browser(), GURL(pageURL));
  ASSERT_TRUE(RunJavascriptTest("runBidiChecker",
                                *Value::CreateStringValue(pageURL),
                                *Value::CreateBooleanValue(isRTL)));
}

// WebUIBidiCheckerBrowserTestFakeBidi
#ifndef OS_WIN
WebUIBidiCheckerBrowserTestFakeBidi::~WebUIBidiCheckerBrowserTestFakeBidi() {}

WebUIBidiCheckerBrowserTestFakeBidi::WebUIBidiCheckerBrowserTestFakeBidi() {}

void WebUIBidiCheckerBrowserTestFakeBidi::SetUpInProcessBrowserTestFixture() {
  WebUIBidiCheckerBrowserTest::SetUpInProcessBrowserTestFixture();
  FilePath pak_path;
  PathService::Get(base::FILE_MODULE, &pak_path);
  pak_path = pak_path.DirName();
  pak_path = pak_path.Append("pseudo_locales");
  pak_path = pak_path.Append("fake-bidi");
  pak_path = pak_path.ReplaceExtension("pak");
  ResourceBundle::GetSharedInstance().OverrideLocalePak(pak_path);
  ResourceBundle::ReloadSharedInstance("he");
}

void WebUIBidiCheckerBrowserTestFakeBidi::TearDownInProcessBrowserTestFixture()
{
  WebUIBidiCheckerBrowserTest::TearDownInProcessBrowserTestFixture();
  FilePath file_path;
  ResourceBundle::GetSharedInstance().OverrideLocalePak(file_path);
  ResourceBundle::ReloadSharedInstance("en");
}
#endif // OS_WIN

// Tests

IN_PROC_BROWSER_TEST_F(WebUIBidiCheckerBrowserTest, TestMainHistoryPageLTR) {
  HistoryService* history_service =
      browser()->profile()->GetHistoryService(Profile::IMPLICIT_ACCESS);
  const GURL history_url = GURL("http://www.ynet.co.il");
  history_service->AddPage(history_url, history::SOURCE_BROWSED);
  string16 title;
  ASSERT_TRUE(UTF8ToUTF16("\xD7\x91\xD7\x93\xD7\x99\xD7\xA7\xD7\x94\x21",
                          12,
                          &title));
  history_service->SetPageTitle(history_url, title);
  RunBidiCheckerOnPage(chrome::kChromeUIHistoryURL, false);
}

#ifndef OS_WIN
IN_PROC_BROWSER_TEST_F(WebUIBidiCheckerBrowserTestFakeBidi,
                       TestMainHistoryPageRTL) {
  HistoryService* history_service =
  browser()->profile()->GetHistoryService(Profile::IMPLICIT_ACCESS);
  const GURL history_url = GURL("http://www.google.com");
  history_service->AddPage(history_url, history::SOURCE_BROWSED);
  string16 title;
  ASSERT_TRUE(UTF8ToUTF16("Google",
                          6,
                          &title));
  history_service->SetPageTitle(history_url, title);
  WebUIBidiCheckerBrowserTest::RunBidiCheckerOnPage(chrome::kChromeUIHistoryURL,
                                                    true);
}
#endif // OS_WIN

IN_PROC_BROWSER_TEST_F(WebUIBidiCheckerBrowserTest, TestAboutPageLTR) {
  RunBidiCheckerOnPage(chrome::kChromeUIAboutURL, false);
}

// Times out. http://code.google.com/p/chromium/issues/detail?id=82896
IN_PROC_BROWSER_TEST_F(WebUIBidiCheckerBrowserTest,
                       DISABLED_TestBookmarksPageLTR) {
  RunBidiCheckerOnPage(chrome::kChromeUIBookmarksURL, false);
}

#ifndef OS_WIN
// Times out. http://code.google.com/p/chromium/issues/detail?id=82896
IN_PROC_BROWSER_TEST_F(WebUIBidiCheckerBrowserTestFakeBidi,
                       DISABLED_TestBookmarksPageRTL) {
  WebUIBidiCheckerBrowserTest::RunBidiCheckerOnPage(
      chrome::kChromeUIBookmarksURL, true);
}
#endif // OS_WIN

IN_PROC_BROWSER_TEST_F(WebUIBidiCheckerBrowserTest, TestBugReportPageLTR) {
  RunBidiCheckerOnPage(chrome::kChromeUIBugReportURL, false);
}

IN_PROC_BROWSER_TEST_F(WebUIBidiCheckerBrowserTest, TestCrashesPageLTR) {
  RunBidiCheckerOnPage(chrome::kChromeUICrashesURL, false);
}

#ifndef OS_WIN
IN_PROC_BROWSER_TEST_F(WebUIBidiCheckerBrowserTestFakeBidi,
                       TestCrashesPageRTL) {
  WebUIBidiCheckerBrowserTest::RunBidiCheckerOnPage(chrome::kChromeUICrashesURL,
                                                    true);
}
#endif // OS_WIN

IN_PROC_BROWSER_TEST_F(WebUIBidiCheckerBrowserTest, TestDownloadsPageLTR) {
  RunBidiCheckerOnPage(chrome::kChromeUIDownloadsURL, false);
}

#ifndef OS_WIN
IN_PROC_BROWSER_TEST_F(WebUIBidiCheckerBrowserTestFakeBidi,
                       TestDownloadsPageRTL) {
  WebUIBidiCheckerBrowserTest::RunBidiCheckerOnPage(
      chrome::kChromeUIDownloadsURL, true);
}
#endif // OS_WIN

IN_PROC_BROWSER_TEST_F(WebUIBidiCheckerBrowserTest, TestNewTabPageLTR) {
  RunBidiCheckerOnPage(chrome::kChromeUINewTabURL, false);
}

#ifndef OS_WIN
IN_PROC_BROWSER_TEST_F(WebUIBidiCheckerBrowserTestFakeBidi, TestNewTabPageRTL) {
  WebUIBidiCheckerBrowserTest::RunBidiCheckerOnPage(chrome::kChromeUINewTabURL,
                                                    true);
}
#endif // OS_WIN

IN_PROC_BROWSER_TEST_F(WebUIBidiCheckerBrowserTest, TestPluginsPageLTR) {
  RunBidiCheckerOnPage(chrome::kChromeUIPluginsURL, false);
}

#ifndef OS_WIN
IN_PROC_BROWSER_TEST_F(WebUIBidiCheckerBrowserTestFakeBidi,
                       TestPluginsPageRTL) {
  WebUIBidiCheckerBrowserTest::RunBidiCheckerOnPage(chrome::kChromeUIPluginsURL,
                                                    true);
}
#endif // OS_WIN

IN_PROC_BROWSER_TEST_F(WebUIBidiCheckerBrowserTest, TestSettingsPageLTR) {
  RunBidiCheckerOnPage(chrome::kChromeUISettingsURL, false);
}

#ifndef OS_WIN
IN_PROC_BROWSER_TEST_F(WebUIBidiCheckerBrowserTestFakeBidi,
                       TestSettingsPageRTL) {
  WebUIBidiCheckerBrowserTest::RunBidiCheckerOnPage(
      chrome::kChromeUISettingsURL, true);
}
#endif // OS_WIN

IN_PROC_BROWSER_TEST_F(WebUIBidiCheckerBrowserTest,
                       TestSettingsAutofillPageLTR) {
  std::string url(chrome::kChromeUISettingsURL);
  url += std::string(chrome::kAutofillSubPage);

  autofill_test::DisableSystemServices(browser()->profile());
  AutofillProfile profile;
  autofill_test::SetProfileInfo(
      &profile,
      "\xD7\x9E\xD7\xA9\xD7\x94",
      "\xD7\x91",
      "\xD7\x9B\xD7\x94\xD7\x9F",
      "moshe.b.cohen@biditest.com",
      "\xD7\x91\xD7\x93\xD7\x99\xD7\xA7\xD7\x94\x20\xD7\x91\xD7\xA2\xD7\x9E",
      "\xD7\x93\xD7\xA8\xD7\x9A\x20\xD7\x9E\xD7\xA0\xD7\x97\xD7\x9D\x20\xD7\x91\xD7\x92\xD7\x99\xD7\x9F\x20\x32\x33",
      "\xD7\xA7\xD7\x95\xD7\x9E\xD7\x94\x20\x32\x36",
      "\xD7\xAA\xD7\x9C\x20\xD7\x90\xD7\x91\xD7\x99\xD7\x91",
      "",
      "66183",
      "\xD7\x99\xD7\xA9\xD7\xA8\xD7\x90\xD7\x9C",
      "0000",
      "0000");

  PersonalDataManager* personal_data_manager =
    browser()->profile()->GetPersonalDataManager();
  ASSERT_TRUE(personal_data_manager);

  personal_data_manager->AddProfile(profile);

  RunBidiCheckerOnPage(url.c_str(), false);
}

#ifndef OS_WIN
IN_PROC_BROWSER_TEST_F(WebUIBidiCheckerBrowserTestFakeBidi,
                       TestSettingsAutofillPageRTL) {
  std::string url(chrome::kChromeUISettingsURL);
  url += std::string(chrome::kAutofillSubPage);

  autofill_test::DisableSystemServices(browser()->profile());
  AutofillProfile profile;
  autofill_test::SetProfileInfo(
                                &profile,
                                "Milton",
                                "C.",
                                "Waddams",
                                "red.swingline@initech.com",
                                "Initech",
                                "4120 Freidrich Lane",
                                "Basement",
                                "Austin",
                                "Texas",
                                "78744",
                                "United States",
                                "5125551234",
                                "5125550000");

  PersonalDataManager* personal_data_manager =
    browser()->profile()->GetPersonalDataManager();
  ASSERT_TRUE(personal_data_manager);

  personal_data_manager->AddProfile(profile);

  WebUIBidiCheckerBrowserTest::RunBidiCheckerOnPage(url.c_str(), true);
}
#endif // OS_WIN
