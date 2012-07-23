// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/image/canvas_image_source.h"

#include "base/logging.h"
#include "ui/gfx/canvas.h"
#include "ui/base/layout.h"

namespace gfx {

////////////////////////////////////////////////////////////////////////////////
// CanvasImageSource

CanvasImageSource::CanvasImageSource(const gfx::Size& size, bool is_opaque)
    : size_(size),
      is_opaque_(is_opaque) {
}

gfx::ImageSkiaRep CanvasImageSource::GetImageForScale(
    ui::ScaleFactor scale_factor) {
  gfx::Canvas canvas(size_, scale_factor, is_opaque_);
  Draw(&canvas);
  return canvas.ExtractImageSkiaRep();
}

}  // namespace gfx
