// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/extensions/manifest_tests/extension_manifest_test.h"

#include "chrome/common/extensions/extension.h"
#include "chrome/common/extensions/extension_manifest_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace errors = extension_manifest_errors;

TEST_F(ExtensionManifestTest, DefaultPathForExtent) {
  scoped_refptr<Extension> extension(
      LoadAndExpectSuccess("default_path_for_extent.json"));

  ASSERT_EQ(1u, extension->web_extent().patterns().size());
  EXPECT_EQ("/*", extension->web_extent().patterns().begin()->path());
  EXPECT_TRUE(extension->web_extent().MatchesURL(
      GURL("http://www.google.com/monkey")));
}

TEST_F(ExtensionManifestTest, DefaultLocale) {
  LoadAndExpectError("default_locale_invalid.json",
                     errors::kInvalidDefaultLocale);

  scoped_refptr<Extension> extension(
      LoadAndExpectSuccess("default_locale_valid.json"));
  EXPECT_EQ("de-AT", extension->default_locale());
}
