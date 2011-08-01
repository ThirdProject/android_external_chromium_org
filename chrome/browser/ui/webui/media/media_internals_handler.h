// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_MEDIA_MEDIA_INTERNALS_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_MEDIA_MEDIA_INTERNALS_HANDLER_H_
#pragma once

#include "chrome/browser/ui/webui/chrome_web_ui.h"

class MediaInternalsProxy;

namespace base {
class ListValue;
}

// This class handles messages to and from MediaInternalsUI.
// It does all its work on the IO thread through the proxy below.
class MediaInternalsMessageHandler : public WebUIMessageHandler {
 public:
  MediaInternalsMessageHandler();
  virtual ~MediaInternalsMessageHandler();

  // WebUIMessageHandler implementation.
  virtual WebUIMessageHandler* Attach(WebUI* web_ui) OVERRIDE;
  virtual void RegisterMessages() OVERRIDE;

  // Javascript message handlers.
  void OnGetEverything(const base::ListValue* list);

  // MediaInternals message handlers.
  void OnUpdate(const string16& update);

 private:
  scoped_refptr<MediaInternalsProxy> proxy_;

  DISALLOW_COPY_AND_ASSIGN(MediaInternalsMessageHandler);
};

#endif  // CHROME_BROWSER_UI_WEBUI_MEDIA_MEDIA_INTERNALS_HANDLER_H_
