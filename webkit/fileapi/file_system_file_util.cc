// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/fileapi/file_system_file_util.h"

namespace fileapi {

base::FilePath FileSystemFileUtil::EmptyFileEnumerator::Next() {
  return base::FilePath();
}

int64 FileSystemFileUtil::EmptyFileEnumerator::Size() {
  return 0;
}

base::Time FileSystemFileUtil::EmptyFileEnumerator::LastModifiedTime() {
  return base::Time();
}

bool FileSystemFileUtil::EmptyFileEnumerator::IsDirectory() {
  return false;
}

}  // namespace fileapi
