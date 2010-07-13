// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/status/power_menu_button.h"

#include "chrome/browser/browser.h"
#include "chrome/browser/browser_window.h"
#include "chrome/browser/chromeos/cros/cros_in_process_browser_test.h"
#include "chrome/browser/chromeos/cros/mock_power_library.h"
#include "chrome/browser/chromeos/frame/browser_view.h"
#include "chrome/browser/chromeos/status/browser_status_area_view.h"
#include "chrome/browser/chromeos/view_ids.h"
#include "grit/theme_resources.h"

namespace chromeos {
using ::testing::AnyNumber;
using ::testing::InvokeWithoutArgs;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::_;

class PowerMenuButtonTest : public CrosInProcessBrowserTest {
 protected:
  PowerMenuButtonTest() : CrosInProcessBrowserTest() {}

  virtual void SetUpInProcessBrowserTestFixture() {
    InitStatusAreaMocks();
    SetStatusAreaMocksExpectations();
  }

  PowerMenuButton* GetPowerMenuButton() {
    BrowserView* view = static_cast<BrowserView*>(browser()->window());
    PowerMenuButton* power = static_cast<BrowserStatusAreaView*>(view->
        GetViewByID(VIEW_ID_STATUS_AREA))->power_view();
    return power;
  }

  int CallPowerChangedAndGetIconId() {
    PowerMenuButton* power = GetPowerMenuButton();
    power->PowerChanged(mock_power_library_);
    return power->icon_id();
  }
};

IN_PROC_BROWSER_TEST_F(PowerMenuButtonTest, BatteryMissingTest) {
  EXPECT_CALL(*mock_power_library_, battery_is_present())
      .WillRepeatedly((Return(false)));
  EXPECT_EQ(IDR_STATUSBAR_BATTERY_MISSING, CallPowerChangedAndGetIconId());
}

IN_PROC_BROWSER_TEST_F(PowerMenuButtonTest, BatteryChargedTest) {
  EXPECT_CALL(*mock_power_library_, battery_is_present())
      .WillRepeatedly((Return(true)));
  EXPECT_CALL(*mock_power_library_, battery_fully_charged())
      .WillRepeatedly((Return(true)));
  EXPECT_CALL(*mock_power_library_, line_power_on())
      .WillRepeatedly((Return(true)));
  EXPECT_EQ(IDR_STATUSBAR_BATTERY_CHARGED, CallPowerChangedAndGetIconId());
}

IN_PROC_BROWSER_TEST_F(PowerMenuButtonTest, BatteryChargingTest) {
  EXPECT_CALL(*mock_power_library_, battery_is_present())
      .WillRepeatedly((Return(true)));
  EXPECT_CALL(*mock_power_library_, battery_fully_charged())
      .WillRepeatedly((Return(false)));
  EXPECT_CALL(*mock_power_library_, line_power_on())
      .WillRepeatedly((Return(true)));

  // Test the 12 battery charging states.
  // NOTE: Use an array rather than just calculating a resource number to avoid
  // creating implicit ordering dependencies on the resource values.
  static const int kChargingImages[] = {
    IDR_STATUSBAR_BATTERY_CHARGING_1,
    IDR_STATUSBAR_BATTERY_CHARGING_2,
    IDR_STATUSBAR_BATTERY_CHARGING_3,
    IDR_STATUSBAR_BATTERY_CHARGING_4,
    IDR_STATUSBAR_BATTERY_CHARGING_5,
    IDR_STATUSBAR_BATTERY_CHARGING_6,
    IDR_STATUSBAR_BATTERY_CHARGING_7,
    IDR_STATUSBAR_BATTERY_CHARGING_8,
    IDR_STATUSBAR_BATTERY_CHARGING_9,
    IDR_STATUSBAR_BATTERY_CHARGING_10,
    IDR_STATUSBAR_BATTERY_CHARGING_11,
    IDR_STATUSBAR_BATTERY_CHARGING_12,
  };
  size_t id = 0;
  for (float percent = 6.0; percent < 100.0; percent += 8.0) {
    EXPECT_CALL(*mock_power_library_, battery_percentage())
        .WillRepeatedly((Return(percent)));
    ASSERT_LT(id, arraysize(kChargingImages));
    EXPECT_EQ(kChargingImages[id], CallPowerChangedAndGetIconId());
    id++;
  }
}

IN_PROC_BROWSER_TEST_F(PowerMenuButtonTest, BatteryDischargingTest) {
  EXPECT_CALL(*mock_power_library_, battery_is_present())
      .WillRepeatedly((Return(true)));
  EXPECT_CALL(*mock_power_library_, battery_fully_charged())
      .WillRepeatedly((Return(false)));
  EXPECT_CALL(*mock_power_library_, line_power_on())
      .WillRepeatedly((Return(false)));

  // Test the 12 battery discharing states.
  // NOTE: Use an array rather than just calculating a resource number to avoid
  // creating implicit ordering dependencies on the resource values.
  static const int kDischargingImages[] = {
    IDR_STATUSBAR_BATTERY_DISCHARGING_1,
    IDR_STATUSBAR_BATTERY_DISCHARGING_2,
    IDR_STATUSBAR_BATTERY_DISCHARGING_3,
    IDR_STATUSBAR_BATTERY_DISCHARGING_4,
    IDR_STATUSBAR_BATTERY_DISCHARGING_5,
    IDR_STATUSBAR_BATTERY_DISCHARGING_6,
    IDR_STATUSBAR_BATTERY_DISCHARGING_7,
    IDR_STATUSBAR_BATTERY_DISCHARGING_8,
    IDR_STATUSBAR_BATTERY_DISCHARGING_9,
    IDR_STATUSBAR_BATTERY_DISCHARGING_10,
    IDR_STATUSBAR_BATTERY_DISCHARGING_11,
    IDR_STATUSBAR_BATTERY_DISCHARGING_12,
  };
  size_t id = 0;
  for (float percent = 6.0; percent < 100.0; percent += 8.0) {
    EXPECT_CALL(*mock_power_library_, battery_percentage())
        .WillRepeatedly((Return(percent)));
    ASSERT_LT(id, arraysize(kDischargingImages));
    EXPECT_EQ(kDischargingImages[id], CallPowerChangedAndGetIconId());
    id++;
  }
}

}  // namespace chromeos
