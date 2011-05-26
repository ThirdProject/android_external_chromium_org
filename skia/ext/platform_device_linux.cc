// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "skia/ext/platform_device.h"

namespace skia {

PlatformDevice::PlatformDevice(const SkBitmap& bitmap)
    : SkDevice(NULL, bitmap, /*isForLayer=*/false) {
  SetPlatformDevice(this, this);
}

bool PlatformDevice::IsNativeFontRenderingAllowed() {
  return true;
}

void PlatformDevice::EndPlatformPaint() {
  // We don't need to do anything on Linux here.
}

void PlatformDevice::DrawToNativeContext(PlatformSurface surface, int x, int y,
                                         const PlatformRect* src_rect) {
  // Should never be called on Linux.
  SkASSERT(false);
}

}  // namespace skia
