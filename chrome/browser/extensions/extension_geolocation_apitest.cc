// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/geolocation/location_arbitrator.h"
#include "chrome/browser/geolocation/mock_location_provider.h"

class GeolocationApiTest : public ExtensionApiTest {
 public:
  // InProcessBrowserTest
  void SetUpInProcessBrowserTestFixture() {
    ExtensionApiTest::SetUpInProcessBrowserTestFixture();
    GeolocationArbitrator::SetProviderFactoryForTest(
        &NewAutoSuccessMockLocationProvider);
  }
};

IN_PROC_BROWSER_TEST_F(GeolocationApiTest,
                       FLAKY_ExtensionGeolocationAccessFail) {
  // Test that geolocation cannot be accessed from extension without permission.
  ASSERT_TRUE(RunExtensionTest("geolocation/no_permission")) << message_;
}

IN_PROC_BROWSER_TEST_F(GeolocationApiTest, ExtensionGeolocationAccessPass) {
  // Test that geolocation can be accessed from extension with permission.
  ASSERT_TRUE(RunExtensionTest("geolocation/has_permission")) << message_;
}
