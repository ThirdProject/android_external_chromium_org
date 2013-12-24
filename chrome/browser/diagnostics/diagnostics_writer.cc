// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/diagnostics/diagnostics_writer.h"

#include "build/build_config.h"

#if defined(OS_POSIX)
#include <stdio.h>
#include <unistd.h>
#endif

#include <string>

#include "base/basictypes.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/logging.h"
#include "base/strings/string16.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/common/chrome_switches.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/ui_base_paths.h"

namespace diagnostics {

// This is a minimalistic interface to wrap the platform console.
class SimpleConsole {
 public:
  enum Color {
    DEFAULT,
    RED,
    GREEN,
  };

  virtual ~SimpleConsole() {}

  // Init must be called before using any other method. If it returns
  // false there will be no console output.
  virtual bool Init() = 0;

  // Writes a string to the console with the current color.
  virtual bool Write(const base::string16& text) = 0;

  // Called when the program is about to exit.
  virtual void OnQuit() = 0;

  // Sets the foreground text color.
  virtual bool SetColor(Color color) = 0;

  // Create an appropriate SimpleConsole instance.  May return NULL if there is
  // no implementation for the current platform.
  static SimpleConsole* Create();
};

#if defined(OS_WIN)
namespace {

// Wrapper for the windows console operating in high-level IO mode.
class WinConsole : public SimpleConsole {
 public:
  // The ctor allocates a console. This avoids having to ask the user to start
  // chrome from a command prompt.
  WinConsole()
      : std_out_(INVALID_HANDLE_VALUE),
        std_in_(INVALID_HANDLE_VALUE) {
    ::AllocConsole();
  }

  virtual ~WinConsole() {
    ::FreeConsole();
  }

  virtual bool Init() {
    return SetIOHandles();
  }

  virtual bool Write(const base::string16& txt) {
    DWORD sz = txt.size();
    return (TRUE == ::WriteConsoleW(std_out_, txt.c_str(), sz, &sz, NULL));
  }

  // Reads a string from the console. Internally it is limited to 256
  // characters.
  virtual void OnQuit() {
    // Block here so the user can see the results.
    SetColor(SimpleConsole::DEFAULT);
    Write(L"Press [enter] to continue\n");
    wchar_t buf[256];
    DWORD read = arraysize(buf);
    ::ReadConsoleW(std_in_, buf, read, &read, NULL);
  }

  // Sets the foreground and background color.
  virtual bool SetColor(Color color) {
    uint16 color_combo = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE |
                         FOREGROUND_INTENSITY;
    switch (color) {
      case RED:
        color_combo = FOREGROUND_RED | FOREGROUND_INTENSITY;
        break;
      case GREEN:
        color_combo = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        break;
      case DEFAULT:
        break;
      default:
        NOTREACHED();
    }
    return (TRUE == ::SetConsoleTextAttribute(std_out_, color_combo));
  }

 private:
  bool SetIOHandles() {
    std_out_ = ::GetStdHandle(STD_OUTPUT_HANDLE);
    std_in_ = ::GetStdHandle(STD_INPUT_HANDLE);
    return ((std_out_ != INVALID_HANDLE_VALUE) &&
            (std_in_ != INVALID_HANDLE_VALUE));
  }

  // The input and output handles to the screen. They seem to be
  // implemented as pipes but they have non-documented protocol.
  HANDLE std_out_;
  HANDLE std_in_;

  DISALLOW_COPY_AND_ASSIGN(WinConsole);
};

}  // namespace

SimpleConsole* SimpleConsole::Create() { return new WinConsole(); }

#elif defined(OS_POSIX)
namespace {

class PosixConsole : public SimpleConsole {
 public:
  PosixConsole() : use_color_(false) {}

  virtual bool Init() OVERRIDE {
    // Technically, we should also check the terminal capabilities before using
    // color, but in practice this is unlikely to be an issue.
    use_color_ = isatty(STDOUT_FILENO);
    return true;
  }

  virtual bool Write(const base::string16& text) OVERRIDE {
    // We're assuming that the terminal is using UTF-8 encoding.
    printf("%s", base::UTF16ToUTF8(text).c_str());
    return true;
  }

  virtual void OnQuit() OVERRIDE {
    // The "press enter to continue" prompt isn't very unixy, so only do that on
    // Windows.
  }

  virtual bool SetColor(Color color) OVERRIDE {
    if (!use_color_)
      return false;

    const char* code = "\033[m";
    switch (color) {
      case RED:
        code = "\033[1;31m";
        break;
      case GREEN:
        code = "\033[1;32m";
        break;
      case DEFAULT:
        break;
      default:
        NOTREACHED();
    }
    printf("%s", code);
    return true;
  }

 private:
  bool use_color_;

  DISALLOW_COPY_AND_ASSIGN(PosixConsole);
};

}  // namespace

SimpleConsole* SimpleConsole::Create() { return new PosixConsole(); }

#else  // !defined(OS_WIN) && !defined(OS_POSIX)
SimpleConsole* SimpleConsole::Create() { return NULL; }
#endif

///////////////////////////////////////////////////////////
//  DiagnosticsWriter

DiagnosticsWriter::DiagnosticsWriter(FormatType format)
    : failures_(0), format_(format) {
  // Only create consoles for non-log output.
  if (format_ != LOG) {
    console_.reset(SimpleConsole::Create());
    console_->Init();
  }
}

DiagnosticsWriter::~DiagnosticsWriter() {
  if (console_.get())
    console_->OnQuit();
}

bool DiagnosticsWriter::WriteInfoLine(const std::string& info_text) {
  if (format_ == LOG) {
    LOG(WARNING) << info_text;
    return true;
  } else {
    if (console_.get()) {
      console_->SetColor(SimpleConsole::DEFAULT);
      console_->Write(base::UTF8ToUTF16(info_text + "\n"));
    }
  }
  return true;
}

void DiagnosticsWriter::OnTestFinished(int index, DiagnosticsModel* model) {
  const DiagnosticsModel::TestInfo& test_info = model->GetTest(index);
  bool success = (DiagnosticsModel::TEST_OK == test_info.GetResult());
  WriteResult(success,
              test_info.GetName(),
              test_info.GetTitle(),
              test_info.GetOutcomeCode(),
              test_info.GetAdditionalInfo());
}

void DiagnosticsWriter::OnAllTestsDone(DiagnosticsModel* model) {
  WriteInfoLine(
      base::StringPrintf("Finished %d tests.", model->GetTestRunCount()));
}

void DiagnosticsWriter::OnRecoveryFinished(int index, DiagnosticsModel* model) {
  const DiagnosticsModel::TestInfo& test_info = model->GetTest(index);
  WriteInfoLine("Finished Recovery for: " + test_info.GetTitle());
}

void DiagnosticsWriter::OnAllRecoveryDone(DiagnosticsModel* model) {
  WriteInfoLine("Finished All Recovery operations.");
}

bool DiagnosticsWriter::WriteResult(bool success,
                                    const std::string& id,
                                    const std::string& name,
                                    int outcome_code,
                                    const std::string& extra) {
  std::string result;
  SimpleConsole::Color color;

  if (success) {
    result = "[PASS] ";
    color = SimpleConsole::GREEN;
  } else {
    color = SimpleConsole::RED;
    result = "[FAIL] ";
    failures_++;
  }

  if (format_ != LOG) {
    if (console_.get()) {
      console_->SetColor(color);
      console_->Write(base::ASCIIToUTF16(result));
    }
    if (format_ == MACHINE) {
      return WriteInfoLine(base::StringPrintf(
          "%03d %s (%s)", outcome_code, id.c_str(), extra.c_str()));
    } else {
      return WriteInfoLine(name + "\n       " + extra + "\n");
    }
  } else {
    if (!success) {
      // For log output, we only care about the tests that failed.
      return WriteInfoLine(base::StringPrintf("%s%03d %s (%s)",
                                              result.c_str(),
                                              outcome_code,
                                              id.c_str(),
                                              extra.c_str()));
    }
  }
  return true;
}

}  // namespace diagnostics
