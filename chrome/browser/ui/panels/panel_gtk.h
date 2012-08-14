// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_PANELS_PANEL_GTK_H_
#define CHROME_BROWSER_UI_PANELS_PANEL_GTK_H_

#include <gtk/gtk.h>
#include <map>

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "base/timer.h"
#include "chrome/browser/ui/panels/native_panel.h"
#include "ui/base/animation/animation_delegate.h"
#include "ui/base/gtk/gtk_signal.h"
#include "ui/base/x/active_window_watcher_x_observer.h"
#include "ui/gfx/rect.h"

class Panel;
class PanelBoundsAnimation;
class PanelTitlebarGtk;
class PanelDragGtk;
class GtkNativePanelTesting;

namespace gfx {
class Image;
}

// An implementation of the native panel in GTK.
class PanelGtk : public NativePanel,
                 public ui::ActiveWindowWatcherXObserver,
                 public ui::AnimationDelegate {
 public:
  enum PaintState {
    PAINT_AS_ACTIVE,
    PAINT_AS_INACTIVE,
    PAINT_AS_MINIMIZED,
    PAINT_FOR_ATTENTION
  };

  PanelGtk(Panel* panel, const gfx::Rect& bounds);
  virtual ~PanelGtk();

  void Init();

  // Overridden from NativePanel.
  virtual void ShowPanel() OVERRIDE;
  virtual void ShowPanelInactive() OVERRIDE;
  virtual gfx::Rect GetPanelBounds() const OVERRIDE;
  virtual void SetPanelBounds(const gfx::Rect& bounds) OVERRIDE;
  virtual void SetPanelBoundsInstantly(const gfx::Rect& bounds) OVERRIDE;
  virtual void ClosePanel() OVERRIDE;
  virtual void ActivatePanel() OVERRIDE;
  virtual void DeactivatePanel() OVERRIDE;
  virtual bool IsPanelActive() const OVERRIDE;
  virtual void PreventActivationByOS(bool prevent_activation) OVERRIDE;
  virtual gfx::NativeWindow GetNativePanelHandle() OVERRIDE;
  virtual void UpdatePanelTitleBar() OVERRIDE;
  virtual void UpdatePanelLoadingAnimations(bool should_animate) OVERRIDE;
  virtual FindBar* CreatePanelFindBar() OVERRIDE;
  virtual void NotifyPanelOnUserChangedTheme() OVERRIDE;
  virtual void PanelCut() OVERRIDE;
  virtual void PanelCopy() OVERRIDE;
  virtual void PanelPaste() OVERRIDE;
  virtual void DrawAttention(bool draw_attention) OVERRIDE;
  virtual bool IsDrawingAttention() const OVERRIDE;
  virtual bool PreHandlePanelKeyboardEvent(
      const content::NativeWebKeyboardEvent& event,
      bool* is_keyboard_shortcut) OVERRIDE;
  virtual void HandlePanelKeyboardEvent(
      const content::NativeWebKeyboardEvent& event) OVERRIDE;
  virtual void FullScreenModeChanged(bool is_full_screen) OVERRIDE;
  virtual void PanelExpansionStateChanging(
      Panel::ExpansionState old_state,
      Panel::ExpansionState new_state) OVERRIDE;
  virtual void AttachWebContents(content::WebContents* contents) OVERRIDE;
  virtual void DetachWebContents(content::WebContents* contents) OVERRIDE;
  virtual Browser* GetPanelBrowser() const OVERRIDE;
  // These sizes are in screen coordinates.
  virtual gfx::Size WindowSizeFromContentSize(
      const gfx::Size& content_size) const OVERRIDE;
  virtual gfx::Size ContentSizeFromWindowSize(
      const gfx::Size& window_size) const OVERRIDE;
  virtual int TitleOnlyHeight() const OVERRIDE;
  virtual void EnsurePanelFullyVisible() OVERRIDE;
  virtual void SetPanelAlwaysOnTop(bool on_top) OVERRIDE;
  virtual void EnableResizeByMouse(bool enable) OVERRIDE;
  virtual void UpdatePanelMinimizeRestoreButtonVisibility() OVERRIDE;

  virtual NativePanelTesting* CreateNativePanelTesting() OVERRIDE;

  // Overridden from ActiveWindowWatcherXObserver.
  virtual void ActiveWindowChanged(GdkWindow* active_window) OVERRIDE;

  bool UsingDefaultTheme() const;

  Panel* panel() const { return panel_.get(); }
  PaintState paint_state() const { return paint_state_; }
  PanelTitlebarGtk* titlebar() const { return titlebar_.get(); }

 private:
  friend class GtkNativePanelTesting;

  // Applies our custom window shape with rounded top corners.
  void UpdateWindowShape(int width, int height);

  // Checks to see if the mouse pointer at |x|, |y| is over the border of the
  // custom frame (a spot that should trigger a window resize). Returns true if
  // it should and sets |edge|.
  bool GetWindowEdge(int x, int y, GdkWindowEdge* edge) const;

  // Connect/disconnect accelerators for keyboard shortcut support.
  void ConnectAccelerators();
  void DisconnectAccelerators();

  // Returns the image to paint the frame.
  const gfx::Image* GetFrameBackground() const;
  const gfx::Image* GetDefaultFrameBackground() const;
  const gfx::Image* GetThemedFrameBackground() const;

  // Animation when panel is first shown.
  void RevealPanel();

  void StartBoundsAnimation(const gfx::Rect& from_bounds,
                            const gfx::Rect& to_bounds);
  bool IsAnimatingBounds() const;

  // Overridden from AnimationDelegate:
  virtual void AnimationEnded(const ui::Animation* animation) OVERRIDE;
  virtual void AnimationProgressed(const ui::Animation* animation) OVERRIDE;

  // Creates helper for handling drags if not already created.
  void EnsureDragHelperCreated();

  void SetBoundsInternal(const gfx::Rect& bounds, bool animate);

  void LoadingAnimationCallback();

  // Returns the size of the window frame around the client content area.
  gfx::Size GetNonClientFrameSize() const;

  // Invalidate window to force repaint.
  void InvalidateWindow();

  // Callback for accelerator activation. |user_data| stores the command id
  // of the matched accelerator.
  static gboolean OnGtkAccelerator(GtkAccelGroup* accel_group,
                                   GObject* acceleratable,
                                   guint keyval,
                                   GdkModifierType modifier,
                                   void* user_data);

  CHROMEGTK_CALLBACK_1(PanelGtk, gboolean, OnMainWindowDeleteEvent,
                       GdkEvent*);
  CHROMEGTK_CALLBACK_0(PanelGtk, void, OnMainWindowDestroy);
  CHROMEGTK_CALLBACK_1(PanelGtk, gboolean, OnConfigure, GdkEventConfigure*);
  // Callback for when the custom frame alignment needs to be redrawn.
  CHROMEGTK_CALLBACK_1(PanelGtk, gboolean, OnCustomFrameExpose,
                       GdkEventExpose*);
  // Key press event callback.
  CHROMEGTK_CALLBACK_1(PanelGtk, gboolean, OnKeyPress, GdkEventKey*);
  // Mouse move and button press callbacks. If mouse hits titlebar,
  // the titlebar gets the event, else the window gets the button press.
  CHROMEGTK_CALLBACK_1(PanelGtk, gboolean, OnMouseMoveEvent,
                       GdkEventMotion*);
  CHROMEGTK_CALLBACK_1(PanelGtk, gboolean, OnButtonPressEvent,
                       GdkEventButton*);
  CHROMEGTK_CALLBACK_1(PanelGtk, gboolean,
                       OnTitlebarButtonPressEvent, GdkEventButton*);
  CHROMEGTK_CALLBACK_1(PanelGtk, gboolean,
                       OnTitlebarButtonReleaseEvent, GdkEventButton*);

  scoped_ptr<Panel> panel_;
  gfx::Rect bounds_;

  // True after panel has been shown.
  bool is_shown_;

  scoped_ptr<PanelDragGtk> drag_helper_;

  // The configure size of the current window, used to figure out whether to
  // ignore later configure events. See OnConfigure() for more information.
  gfx::Size configure_size_;

  // Indicates different painting state, active, drawing attention or else.
  PaintState paint_state_;

  // Indicates that the panel is currently drawing attention.
  bool is_drawing_attention_;

  // Used to animate the bounds change.
  scoped_ptr<PanelBoundsAnimation> bounds_animator_;
  gfx::Rect animation_start_bounds_;

  // This records the bounds set on the last animation progress notification.
  // We need this for the case where a new bounds animation starts before the
  // current one completes. In this case, we want to start the new animation
  // from where the last one left.
  gfx::Rect last_animation_progressed_bounds_;

  // The timer used to update frames for the Loading Animation.
  base::RepeatingTimer<PanelGtk> loading_animation_timer_;

  // The current window cursor.  We set it to a resize cursor when over the
  // custom frame border.  We set it to NULL if we want the default cursor.
  GdkCursor* frame_cursor_;

  // True if the window manager thinks the window is active.  Not all window
  // managers keep track of this state (_NET_ACTIVE_WINDOW), in which case
  // this will always be true.
  bool is_active_;

  // Top level window.
  GtkWindow* window_;
  // GtkAlignment that holds the interior components of the chromium window.
  // This is used to draw the custom frame border and content shadow.
  GtkWidget* window_container_;
  // VBox that holds everything (titlebar, web contents).
  GtkWidget* window_vbox_;
  // EventBox that holds web contents.
  GtkWidget* render_area_event_box_;
  // We insert and remove WebContents GtkWidgets into this expanded.
  GtkWidget* contents_expanded_;

  // The accelerator group used to handle accelerators, owned by this object.
  GtkAccelGroup* accel_group_;

  // The container for the titlebar.
  scoped_ptr<PanelTitlebarGtk> titlebar_;

  DISALLOW_COPY_AND_ASSIGN(PanelGtk);
};

#endif  // CHROME_BROWSER_UI_PANELS_PANEL_GTK_H_
