// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/dom_distiller/core/distilled_page_prefs.h"

#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "components/pref_registry/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace dom_distiller {

namespace {

class TestingObserver : public DistilledPagePrefs::Observer {
 public:
  TestingObserver()
      : font_(DistilledPagePrefs::SANS_SERIF),
        theme_(DistilledPagePrefs::LIGHT) {}

  virtual void OnChangeFontFamily(
      DistilledPagePrefs::FontFamily new_font) OVERRIDE {
    font_ = new_font;
  }

  DistilledPagePrefs::FontFamily GetFontFamily() { return font_; }

  virtual void OnChangeTheme(DistilledPagePrefs::Theme new_theme) OVERRIDE {
    theme_ = new_theme;
  }

  DistilledPagePrefs::Theme GetTheme() { return theme_; }

 private:
  DistilledPagePrefs::FontFamily font_;
  DistilledPagePrefs::Theme theme_;
};

}  // namespace

class DistilledPagePrefsTest : public testing::Test {
 protected:
  virtual void SetUp() OVERRIDE {
    pref_service_.reset(new user_prefs::TestingPrefServiceSyncable());
    DistilledPagePrefs::RegisterProfilePrefs(pref_service_->registry());
    distilled_page_prefs_.reset(new DistilledPagePrefs(pref_service_.get()));
  }

  scoped_ptr<user_prefs::TestingPrefServiceSyncable> pref_service_;
  scoped_ptr<DistilledPagePrefs> distilled_page_prefs_;

 private:
  base::MessageLoop message_loop_;
};

TEST_F(DistilledPagePrefsTest, TestingOnChangeFontIsBeingCalled) {
  TestingObserver obs;
  distilled_page_prefs_->AddObserver(&obs);
  distilled_page_prefs_->SetFontFamily(DistilledPagePrefs::MONOSPACE);
  EXPECT_EQ(DistilledPagePrefs::SANS_SERIF, obs.GetFontFamily());
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(DistilledPagePrefs::MONOSPACE, obs.GetFontFamily());
  distilled_page_prefs_->SetFontFamily(DistilledPagePrefs::SERIF);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(DistilledPagePrefs::SERIF, obs.GetFontFamily());
  distilled_page_prefs_->RemoveObserver(&obs);
}

TEST_F(DistilledPagePrefsTest, TestingMultipleObserversFont) {
  TestingObserver obs;
  distilled_page_prefs_->AddObserver(&obs);
  TestingObserver obs2;
  distilled_page_prefs_->AddObserver(&obs2);
  distilled_page_prefs_->SetFontFamily(DistilledPagePrefs::SERIF);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(DistilledPagePrefs::SERIF, obs.GetFontFamily());
  EXPECT_EQ(DistilledPagePrefs::SERIF, obs2.GetFontFamily());
  distilled_page_prefs_->RemoveObserver(&obs);
  distilled_page_prefs_->SetFontFamily(DistilledPagePrefs::MONOSPACE);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(DistilledPagePrefs::SERIF, obs.GetFontFamily());
  EXPECT_EQ(DistilledPagePrefs::MONOSPACE, obs2.GetFontFamily());
  distilled_page_prefs_->RemoveObserver(&obs2);
}

TEST_F(DistilledPagePrefsTest, TestingOnChangeThemeIsBeingCalled) {
  TestingObserver obs;
  distilled_page_prefs_->AddObserver(&obs);
  distilled_page_prefs_->SetTheme(DistilledPagePrefs::SEPIA);
  EXPECT_EQ(DistilledPagePrefs::LIGHT, obs.GetTheme());
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(DistilledPagePrefs::SEPIA, obs.GetTheme());
  distilled_page_prefs_->SetTheme(DistilledPagePrefs::DARK);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(DistilledPagePrefs::DARK, obs.GetTheme());
  distilled_page_prefs_->RemoveObserver(&obs);
}

TEST_F(DistilledPagePrefsTest, TestingMultipleObserversTheme) {
  TestingObserver obs;
  distilled_page_prefs_->AddObserver(&obs);
  TestingObserver obs2;
  distilled_page_prefs_->AddObserver(&obs2);
  distilled_page_prefs_->SetTheme(DistilledPagePrefs::SEPIA);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(DistilledPagePrefs::SEPIA, obs.GetTheme());
  EXPECT_EQ(DistilledPagePrefs::SEPIA, obs2.GetTheme());
  distilled_page_prefs_->RemoveObserver(&obs);
  distilled_page_prefs_->SetTheme(DistilledPagePrefs::LIGHT);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(DistilledPagePrefs::SEPIA, obs.GetTheme());
  EXPECT_EQ(DistilledPagePrefs::LIGHT, obs2.GetTheme());
  distilled_page_prefs_->RemoveObserver(&obs2);
}

}  // namespace dom_distiller
