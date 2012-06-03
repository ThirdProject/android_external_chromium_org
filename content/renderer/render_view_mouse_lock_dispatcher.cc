// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/render_view_mouse_lock_dispatcher.h"

#include "content/common/view_messages.h"
#include "content/renderer/render_view_impl.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebFrame.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebView.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebWidget.h"

RenderViewMouseLockDispatcher::RenderViewMouseLockDispatcher(
    RenderViewImpl* render_view_impl)
    : content::RenderViewObserver(render_view_impl),
      render_view_impl_(render_view_impl) {
}

RenderViewMouseLockDispatcher::~RenderViewMouseLockDispatcher() {
}

void RenderViewMouseLockDispatcher::SendLockMouseRequest(
    bool unlocked_by_target) {
  bool user_gesture =
      render_view_impl_->webview() &&
      render_view_impl_->webview()->mainFrame() &&
      render_view_impl_->webview()->mainFrame()->isProcessingUserGesture();

  Send(new ViewHostMsg_LockMouse(routing_id(), user_gesture, unlocked_by_target,
                                 false));
}

void RenderViewMouseLockDispatcher::SendUnlockMouseRequest() {
  Send(new ViewHostMsg_UnlockMouse(routing_id()));
}

bool RenderViewMouseLockDispatcher::OnMessageReceived(
    const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(RenderViewMouseLockDispatcher, message)
    IPC_MESSAGE_HANDLER(ViewMsg_LockMouse_ACK, OnMsgLockMouseACK)
    IPC_MESSAGE_FORWARD(ViewMsg_MouseLockLost,
                        static_cast<MouseLockDispatcher*>(this),
                        MouseLockDispatcher::OnMouseLockLost)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void RenderViewMouseLockDispatcher::OnMsgLockMouseACK(bool succeeded) {
  // Notify the base class.
  OnLockMouseACK(succeeded);

  // Mouse Lock removes the system cursor and provides all mouse motion as
  // .movementX/Y values on events all sent to a fixed target. This requires
  // content to specifically request the mode to be entered.
  // Mouse Capture is implicitly given for the duration of a drag event, and
  // sends all mouse events to the initial target of the drag.
  // If Lock is entered it supercedes any in progress Capture.
  if (succeeded && render_view_impl_->webwidget())
    render_view_impl_->webwidget()->mouseCaptureLost();
}
