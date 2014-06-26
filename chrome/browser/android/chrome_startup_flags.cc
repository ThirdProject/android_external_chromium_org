// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <jni.h>

#include "chrome/browser/android/chrome_startup_flags.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/sys_info.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/chrome_version_info.h"
#include "content/public/common/content_switches.h"
#include "media/base/media_switches.h"

namespace {

void SetCommandLineSwitch(const std::string& switch_string) {
  CommandLine* command_line = CommandLine::ForCurrentProcess();
  if (!command_line->HasSwitch(switch_string))
    command_line->AppendSwitch(switch_string);
}

void SetCommandLineSwitchASCII(const std::string& switch_string,
                               const std::string& value) {
  CommandLine* command_line = CommandLine::ForCurrentProcess();
  if (!command_line->HasSwitch(switch_string))
    command_line->AppendSwitchASCII(switch_string, value);
}

}  // namespace

void SetChromeSpecificCommandLineFlags() {
  // Enable prerender with holdback.
  SetCommandLineSwitchASCII(switches::kPrerenderMode,
                            switches::kPrerenderModeSwitchValueAuto);

  // Enable prerender for the omnibox.
  SetCommandLineSwitchASCII(switches::kPrerenderFromOmnibox,
                            switches::kPrerenderFromOmniboxSwitchValueEnabled);

  // Disable syncing favicons on low end devices.
  if (base::SysInfo::IsLowEndDevice())
    SetCommandLineSwitchASCII(switches::kDisableSyncTypes, "Favicon Images");

  // Enable DOM Distiller on local builds, canary and dev-channel.
  chrome::VersionInfo::Channel channel = chrome::VersionInfo::GetChannel();
  if (channel == chrome::VersionInfo::CHANNEL_UNKNOWN ||
      channel == chrome::VersionInfo::CHANNEL_CANARY ||
      channel == chrome::VersionInfo::CHANNEL_DEV) {
    SetCommandLineSwitch(switches::kEnableDomDistiller);
  }
}
