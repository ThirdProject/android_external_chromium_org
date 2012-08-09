// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_ROOT_WINDOW_HOST_DELEGATE_H_
#define UI_AURA_ROOT_WINDOW_HOST_DELEGATE_H_

namespace gfx {
class Size;
}

namespace ui {
class KeyEvent;
class MouseEvent;
class ScrollEvent;
class TouchEvent;
}

namespace aura {

class RootWindow;

// A private interface used by RootWindowHost implementations to communicate
// with their owning RootWindow.
class AURA_EXPORT RootWindowHostDelegate {
 public:
  virtual bool OnHostKeyEvent(ui::KeyEvent* event) = 0;
  virtual bool OnHostMouseEvent(ui::MouseEvent* event) = 0;
  virtual bool OnHostScrollEvent(ui::ScrollEvent* event) = 0;
  virtual bool OnHostTouchEvent(ui::TouchEvent* event) = 0;

  virtual void OnHostLostCapture() = 0;

  virtual void OnHostPaint() = 0;

  virtual void OnHostResized(const gfx::Size& size) = 0;

  virtual float GetDeviceScaleFactor() = 0;

  virtual RootWindow* AsRootWindow() = 0;

 protected:
  virtual ~RootWindowHostDelegate() {}
};

}  // namespace aura

#endif  // UI_AURA_ROOT_WINDOW_HOST_DELEGATE_H_
