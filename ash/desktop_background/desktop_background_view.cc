// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/desktop_background/desktop_background_view.h"

#include "ash/ash_export.h"
#include "ash/shell.h"
#include "ash/shell_window_ids.h"
#include "base/utf_string_conversions.h"
#include "grit/ui_resources.h"
#include "ui/aura/root_window.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/image/image.h"
#include "ui/views/widget/widget.h"

namespace ash {
namespace internal {

////////////////////////////////////////////////////////////////////////////////
// DesktopBackgroundView, public:

DesktopBackgroundView::DesktopBackgroundView() {
  wallpaper_ = *ui::ResourceBundle::GetSharedInstance().GetImageNamed(
      IDR_AURA_WALLPAPER).ToSkBitmap();
  wallpaper_.buildMipMap(false);
}

DesktopBackgroundView::~DesktopBackgroundView() {
}

////////////////////////////////////////////////////////////////////////////////
// DesktopBackgroundView, views::View overrides:

void DesktopBackgroundView::OnPaint(gfx::Canvas* canvas) {
  // Scale the image while maintaining the aspect ratio, cropping as
  // necessary to fill the background. Ideally the image should be larger
  // than the largest display supported, if not we will center it rather than
  // streching to avoid upsampling artifacts (Note that we could tile too, but
  // decided not to do this at the moment).
  gfx::Rect wallpaper_rect(0, 0, wallpaper_.width(), wallpaper_.height());
  if (wallpaper_.width() > width() && wallpaper_.height() > height()) {
    // The dimension with the smallest ratio must be cropped, the other one
    // is preserved. Both are set in gfx::Size cropped_size.
    double horizontal_ratio = static_cast<double>(width()) /
        static_cast<double>(wallpaper_.width());
    double vertical_ratio = static_cast<double>(height()) /
        static_cast<double>(wallpaper_.height());

    gfx::Size cropped_size;
    if (vertical_ratio > horizontal_ratio) {
      cropped_size = gfx::Size(
          static_cast<int>(
              round(static_cast<double>(width()) / vertical_ratio)),
          wallpaper_.height());
    } else {
      cropped_size = gfx::Size(wallpaper_.width(),
          static_cast<int>(
              round(static_cast<double>(height()) / horizontal_ratio)));
    }

    gfx::Rect wallpaper_cropped_rect = wallpaper_rect.Center(cropped_size);
    canvas->DrawBitmapInt(wallpaper_,
       wallpaper_cropped_rect.x(), wallpaper_cropped_rect.y(),
       wallpaper_cropped_rect.width(), wallpaper_cropped_rect.height(),
       0, 0, width(), height(),
       true);
  }
  else {
    // Center the wallpaper in the destination rectangle (Skia will crop
    // as needed).
    canvas->DrawBitmapInt(wallpaper_, (width() - wallpaper_.width()) / 2,
        (height() - wallpaper_.height()) / 2);
  }
}

bool DesktopBackgroundView::OnMousePressed(const views::MouseEvent& event) {
  return true;
}

void DesktopBackgroundView::OnMouseReleased(const views::MouseEvent& event) {
  if (event.IsRightMouseButton())
    Shell::GetInstance()->ShowBackgroundMenu(GetWidget(), event.location());
}

views::Widget* CreateDesktopBackground() {
  views::Widget* desktop_widget = new views::Widget;
  views::Widget::InitParams params(
      views::Widget::InitParams::TYPE_WINDOW_FRAMELESS);
  DesktopBackgroundView* view = new DesktopBackgroundView;
  params.delegate = view;
  params.parent =
      Shell::GetInstance()->GetContainer(
          ash::internal::kShellWindowId_DesktopBackgroundContainer);
  desktop_widget->Init(params);
  desktop_widget->SetContentsView(view);
  desktop_widget->Show();
  desktop_widget->GetNativeView()->SetName("DesktopBackgroundView");
  return desktop_widget;
}

}  // namespace internal
}  // namespace ash
