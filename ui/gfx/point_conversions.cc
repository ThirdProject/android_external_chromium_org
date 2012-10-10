// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/point_conversions.h"

#include "ui/gfx/safe_floor_ceil.h"

namespace gfx {

Point ToFlooredPoint(const PointF& point) {
  int x = ToFlooredInt(point.x());
  int y = ToFlooredInt(point.y());
  return Point(x, y);
}

Point ToCeiledPoint(const PointF& point) {
  int x = ToCeiledInt(point.x());
  int y = ToCeiledInt(point.y());
  return Point(x, y);
}

}  // namespace gfx

