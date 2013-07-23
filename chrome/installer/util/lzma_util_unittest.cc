// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <windows.h>

#include "base/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/path_service.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/installer/util/lzma_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
class LzmaUtilTest : public testing::Test {
 protected:
  virtual void SetUp() {
    ASSERT_TRUE(PathService::Get(chrome::DIR_TEST_DATA, &data_dir_));
    data_dir_ = data_dir_.AppendASCII("installer");
    ASSERT_TRUE(base::PathExists(data_dir_));

    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
  }

  base::ScopedTempDir temp_dir_;

  // The path to input data used in tests.
  base::FilePath data_dir_;
};
};

// Test that we can open archives successfully.
TEST_F(LzmaUtilTest, OpenArchiveTest) {
  base::FilePath archive = data_dir_.AppendASCII("archive1.7z");
  LzmaUtil lzma_util;
  EXPECT_EQ(lzma_util.OpenArchive(archive.value()), NO_ERROR);

  // We allow opening another archive (which will automatically close the first
  // archive).
  archive = data_dir_.AppendASCII("archive2.7z");
  EXPECT_EQ(lzma_util.OpenArchive(archive.value()), NO_ERROR);

  // Explicitly close and open the first archive again.
  lzma_util.CloseArchive();
  archive = data_dir_.AppendASCII("archive1.7z");
  EXPECT_EQ(lzma_util.OpenArchive(archive.value()), NO_ERROR);

  // Make sure non-existent archive returns error.
  archive = data_dir_.AppendASCII("archive.non_existent.7z");
  EXPECT_EQ(lzma_util.OpenArchive(archive.value()), ERROR_FILE_NOT_FOUND);
}

// Test that we can extract archives successfully.
TEST_F(LzmaUtilTest, UnPackTest) {
  base::FilePath extract_dir(temp_dir_.path());
  extract_dir = extract_dir.AppendASCII("UnPackTest");
  ASSERT_FALSE(base::PathExists(extract_dir));
  EXPECT_TRUE(file_util::CreateDirectory(extract_dir));
  ASSERT_TRUE(base::PathExists(extract_dir));

  base::FilePath archive = data_dir_.AppendASCII("archive1.7z");
  LzmaUtil lzma_util;
  EXPECT_EQ(lzma_util.OpenArchive(archive.value()), NO_ERROR);
  std::wstring unpacked_file;
  EXPECT_EQ(lzma_util.UnPack(extract_dir.value(), &unpacked_file),
            NO_ERROR);
  EXPECT_TRUE(base::PathExists(extract_dir.AppendASCII("a.exe")));
  EXPECT_TRUE(unpacked_file == extract_dir.AppendASCII("a.exe").value());

  archive = data_dir_.AppendASCII("archive2.7z");
  EXPECT_EQ(lzma_util.OpenArchive(archive.value()), NO_ERROR);
  EXPECT_EQ(lzma_util.UnPack(extract_dir.value(), &unpacked_file),
            NO_ERROR);
  EXPECT_TRUE(base::PathExists(extract_dir.AppendASCII("b.exe")));
  EXPECT_TRUE(unpacked_file == extract_dir.AppendASCII("b.exe").value());

  lzma_util.CloseArchive();
  archive = data_dir_.AppendASCII("invalid_archive.7z");
  EXPECT_EQ(lzma_util.UnPack(extract_dir.value(), &unpacked_file),
            ERROR_INVALID_HANDLE);
  EXPECT_EQ(lzma_util.OpenArchive(archive.value()), NO_ERROR);
  EXPECT_EQ(lzma_util.UnPack(extract_dir.value(), &unpacked_file),
            ERROR_INVALID_HANDLE);

  archive = data_dir_.AppendASCII("archive3.7z");
  EXPECT_EQ(lzma_util.OpenArchive(archive.value()), NO_ERROR);
  EXPECT_EQ(lzma_util.UnPack(extract_dir.value(), &unpacked_file),
            NO_ERROR);
  EXPECT_TRUE(base::PathExists(extract_dir.AppendASCII("archive\\a.exe")));
  EXPECT_TRUE(base::PathExists(
      extract_dir.AppendASCII("archive\\sub_dir\\text.txt")));
}

// Test the static method that can be used to unpack archives.
TEST_F(LzmaUtilTest, UnPackArchiveTest) {
  base::FilePath extract_dir(temp_dir_.path());
  extract_dir = extract_dir.AppendASCII("UnPackArchiveTest");
  ASSERT_FALSE(base::PathExists(extract_dir));
  EXPECT_TRUE(file_util::CreateDirectory(extract_dir));
  ASSERT_TRUE(base::PathExists(extract_dir));

  base::FilePath archive = data_dir_.AppendASCII("archive1.7z");
  std::wstring unpacked_file;
  EXPECT_EQ(LzmaUtil::UnPackArchive(archive.value(), extract_dir.value(),
                                    &unpacked_file), NO_ERROR);
  EXPECT_TRUE(base::PathExists(extract_dir.AppendASCII("a.exe")));
  EXPECT_TRUE(unpacked_file == extract_dir.AppendASCII("a.exe").value());

  archive = data_dir_.AppendASCII("archive2.7z");
  EXPECT_EQ(LzmaUtil::UnPackArchive(archive.value(), extract_dir.value(),
                                    &unpacked_file), NO_ERROR);
  EXPECT_TRUE(base::PathExists(extract_dir.AppendASCII("b.exe")));
  EXPECT_TRUE(unpacked_file == extract_dir.AppendASCII("b.exe").value());

  archive = data_dir_.AppendASCII("invalid_archive.7z");
  EXPECT_NE(LzmaUtil::UnPackArchive(archive.value(), extract_dir.value(),
                                    &unpacked_file), NO_ERROR);
}
