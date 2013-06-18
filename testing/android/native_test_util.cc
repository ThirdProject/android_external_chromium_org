// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/android/native_test_util.h"

#include "base/file_util.h"
#include "base/files/file_path.h"
#include "base/strings/string_tokenizer.h"
#include "base/strings/string_util.h"

namespace {

void ParseArgsFromString(const std::string& command_line,
                         std::vector<std::string>* args) {
  base::StringTokenizer tokenizer(command_line, kWhitespaceASCII);
  tokenizer.set_quote_chars("\"");
  while (tokenizer.GetNext()) {
    std::string token;
    RemoveChars(tokenizer.token(), "\"", &token);
    args->push_back(token);
  }
}

}  // namespace

namespace testing {
namespace native_test_util {

void ParseArgsFromCommandLineFile(
    const char* path, std::vector<std::string>* args) {
  base::FilePath command_line(path);
  std::string command_line_string;
  if (file_util::ReadFileToString(command_line, &command_line_string)) {
    ParseArgsFromString(command_line_string, args);
  }
}

int ArgsToArgv(const std::vector<std::string>& args,
                std::vector<char*>* argv) {
  // We need to pass in a non-const char**.
  int argc = args.size();

  argv->resize(argc + 1);
  for (int i = 0; i < argc; ++i) {
    (*argv)[i] = const_cast<char*>(args[i].c_str());
  }
  (*argv)[argc] = NULL;  // argv must be NULL terminated.

  return argc;
}

}  // namespace native_test_util
}  // namespace testing
