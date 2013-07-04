// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/guestview/webview/webview_constants.h"

namespace webview {

// Events.
const char kEventClose[] = "webview.onClose";
const char kEventConsoleMessage[] = "webview.onConsoleMessage";
const char kEventContentLoad[] = "webview.onContentLoad";
const char kEventLoadAbort[] = "webview.onLoadAbort";
const char kEventLoadCommit[] = "webview.onLoadCommit";
const char kEventLoadRedirect[] = "webview.onLoadRedirect";
const char kEventLoadStart[] = "webview.onLoadStart";
const char kEventLoadStop[] = "webview.onLoadStop";

// Parameters/properties on events.
const char kLevel[] = "level";
const char kLine[] = "line";
const char kMessage[] = "message";
const char kNewURL[] = "newUrl";
const char kOldURL[] = "oldUrl";
const char kSourceId[] = "sourceId";

// Internal parameters/properties on events.
const char kInternalCurrentEntryIndex[] = "currentEntryIndex";
const char kInternalEntryCount[] = "entryCount";
const char kInternalProcessId[] = "processId";

}  // namespace webview
