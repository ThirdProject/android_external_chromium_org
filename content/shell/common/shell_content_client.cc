// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/common/shell_content_client.h"

#include "base/command_line.h"
#include "base/strings/string_piece.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/common/content_switches.h"
#include "content/shell/common/shell_switches.h"
#include "grit/shell_resources.h"
#include "grit/webkit_resources.h"
#include "grit/webkit_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "webkit/common/user_agent/user_agent_util.h"

namespace content {

ShellContentClient::~ShellContentClient() {
}

std::string ShellContentClient::GetUserAgent() const {
  std::string product = "Chrome/" CONTENT_SHELL_VERSION;
  CommandLine* command_line = CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kUseMobileUserAgent))
    product += " Mobile";
  return webkit_glue::BuildUserAgentFromProduct(product);
}

string16 ShellContentClient::GetLocalizedString(int message_id) const {
  if (CommandLine::ForCurrentProcess()->HasSwitch(switches::kDumpRenderTree)) {
    switch (message_id) {
      case IDS_FORM_OTHER_DATE_LABEL:
        return ASCIIToUTF16("<<OtherDateLabel>>");
      case IDS_FORM_OTHER_MONTH_LABEL:
        return ASCIIToUTF16("<<OtherMonthLabel>>");
      case IDS_FORM_OTHER_TIME_LABEL:
        return ASCIIToUTF16("<<OtherTimeLabel>>");
      case IDS_FORM_OTHER_WEEK_LABEL:
        return ASCIIToUTF16("<<OtherWeekLabel>>");
      case IDS_FORM_CALENDAR_CLEAR:
        return ASCIIToUTF16("<<CalendarClear>>");
      case IDS_FORM_CALENDAR_TODAY:
        return ASCIIToUTF16("<<CalendarToday>>");
      case IDS_FORM_THIS_MONTH_LABEL:
        return ASCIIToUTF16("<<ThisMonthLabel>>");
      case IDS_FORM_THIS_WEEK_LABEL:
        return ASCIIToUTF16("<<ThisWeekLabel>>");
    }
  }
  return l10n_util::GetStringUTF16(message_id);
}

base::StringPiece ShellContentClient::GetDataResource(
    int resource_id,
    ui::ScaleFactor scale_factor) const {
  if (CommandLine::ForCurrentProcess()->HasSwitch(switches::kDumpRenderTree)) {
    switch (resource_id) {
      case IDR_BROKENIMAGE:
#if defined(OS_MACOSX)
        resource_id = IDR_CONTENT_SHELL_MISSING_IMAGE_PNG;
#else
        resource_id = IDR_CONTENT_SHELL_MISSING_IMAGE_GIF;
#endif
        break;

      case IDR_TEXTAREA_RESIZER:
        resource_id = IDR_CONTENT_SHELL_TEXT_AREA_RESIZE_CORNER_PNG;
        break;
    }
  }
  return ResourceBundle::GetSharedInstance().GetRawDataResourceForScale(
      resource_id, scale_factor);
}

base::RefCountedStaticMemory* ShellContentClient::GetDataResourceBytes(
    int resource_id) const {
  return ResourceBundle::GetSharedInstance().LoadDataResourceBytes(resource_id);
}

gfx::Image& ShellContentClient::GetNativeImageNamed(int resource_id) const {
  return ResourceBundle::GetSharedInstance().GetNativeImageNamed(resource_id);
}

}  // namespace content
