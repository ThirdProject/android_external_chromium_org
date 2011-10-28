// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/screen_rotation.h"

#include "base/debug/trace_event.h"
#include "base/time.h"
#include "ui/gfx/compositor/layer_animation_delegate.h"
#include "ui/gfx/interpolated_transform.h"
#include "ui/gfx/rect.h"
#include "ui/gfx/transform.h"

namespace {

const int k90DegreeTransitionDurationMs = 350;
const int k180DegreeTransitionDurationMs = 550;

base::TimeDelta GetTransitionDuration(int degrees) {
  if (degrees == 180)
    return base::TimeDelta::FromMilliseconds(k180DegreeTransitionDurationMs);
  else if (degrees == 0)
    return base::TimeDelta::FromMilliseconds(0);
  return base::TimeDelta::FromMilliseconds(k90DegreeTransitionDurationMs);
}

}  // namespace

ScreenRotation::ScreenRotation(int degrees)
    : ui::LayerAnimationElement(GetProperties(),
                                GetTransitionDuration(degrees)),
      degrees_(degrees) {
}

ScreenRotation::~ScreenRotation() {
}

void ScreenRotation::OnStart(ui::LayerAnimationDelegate* delegate) {
  //TRACE_EVENT0("ScreenRotation", "init");

  // No rotation required.
  if (degrees_ == 0)
    return;

  const ui::Transform& current_transform = delegate->GetTransformForAnimation();
  const gfx::Rect& bounds = delegate->GetBoundsForAnimation();

  gfx::Point old_pivot;
  gfx::Point new_pivot;

  int width = bounds.width();
  int height = bounds.height();

  switch (degrees_) {
    case 90:
      new_origin_ = new_pivot = gfx::Point(width, 0);
      break;
    case -90:
      new_origin_ = new_pivot = gfx::Point(0, height);
      break;
    case 180:
      new_pivot = old_pivot = gfx::Point(width / 2, height / 2);
      new_origin_.SetPoint(width, height);
      break;
  }

  // Convert points to world space.
  current_transform.TransformPoint(old_pivot);
  current_transform.TransformPoint(new_pivot);
  current_transform.TransformPoint(new_origin_);

  scoped_ptr<ui::InterpolatedTransform> rotation(
      new ui::InterpolatedTransformAboutPivot(
          old_pivot,
          new ui::InterpolatedRotation(0, degrees_)));

  scoped_ptr<ui::InterpolatedTransform> translation(
      new ui::InterpolatedTranslation(
          gfx::Point(0, 0),
          gfx::Point(new_pivot.x() - old_pivot.x(),
                     new_pivot.y() - old_pivot.y())));

  float scale_factor = 0.9f;
  scoped_ptr<ui::InterpolatedTransform> scale_down(
      new ui::InterpolatedScale(1.0f, scale_factor, 0.0f, 0.5f));

  scoped_ptr<ui::InterpolatedTransform> scale_up(
      new ui::InterpolatedScale(1.0f, 1.0f / scale_factor, 0.5f, 1.0f));

  interpolated_transform_.reset(
      new ui::InterpolatedConstantTransform(current_transform));

  scale_up->SetChild(scale_down.release());
  translation->SetChild(scale_up.release());
  rotation->SetChild(translation.release());
  interpolated_transform_->SetChild(rotation.release());
}

void ScreenRotation::OnProgress(double t,
                                ui::LayerAnimationDelegate* delegate) {
  //TRACE_EVENT0("ScreenRotation", "Progress");
  delegate->SetTransformFromAnimation(interpolated_transform_->Interpolate(t));
  delegate->ScheduleDrawForAnimation();
}

void ScreenRotation::OnGetTarget(TargetValue* target) const {
  target->transform = interpolated_transform_->Interpolate(1.0);
}

void ScreenRotation::OnAbort() {
}

// static
const ui::LayerAnimationElement::AnimatableProperties&
ScreenRotation::GetProperties() {
  static LayerAnimationElement::AnimatableProperties properties;
  if (properties.size() == 0) {
    properties.insert(LayerAnimationElement::TRANSFORM);
    properties.insert(LayerAnimationElement::BOUNDS);
  }
  return properties;
}
