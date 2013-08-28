// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_LAUNCHER_LAUNCHER_VIEW_H_
#define ASH_LAUNCHER_LAUNCHER_VIEW_H_

#include <utility>
#include <vector>

#include "ash/launcher/launcher_button_host.h"
#include "ash/launcher/launcher_model_observer.h"
#include "ash/wm/gestures/shelf_gesture_handler.h"
#include "base/observer_list.h"
#include "ui/app_list/views/app_list_drag_and_drop_host.h"
#include "ui/views/animation/bounds_animator_observer.h"
#include "ui/views/context_menu_controller.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/view.h"

namespace views {
class BoundsAnimator;
class MenuModelAdapter;
class MenuRunner;
class ViewModel;
}

namespace ash {

namespace test {
class LauncherViewTestAPI;
}

class LauncherDelegate;
struct LauncherItem;
class LauncherIconObserver;
class LauncherItemDelegateManager;
class LauncherModel;

namespace internal {

class DragImageView;
class LauncherButton;
class LauncherTooltipManager;
class ShelfLayoutManager;
class OverflowBubble;
class OverflowButton;

class ASH_EXPORT LauncherView : public views::View,
                                public LauncherModelObserver,
                                public views::ButtonListener,
                                public LauncherButtonHost,
                                public views::ContextMenuController,
                                public views::FocusTraversable,
                                public views::BoundsAnimatorObserver,
                                public app_list::ApplicationDragAndDropHost {
 public:
  LauncherView(LauncherModel* model,
               LauncherDelegate* delegate,
               ShelfLayoutManager* shelf_layout_manager);
  virtual ~LauncherView();

  LauncherTooltipManager* tooltip_manager() { return tooltip_.get(); }

  LauncherModel* model() { return model_; }

  void Init();

  void OnShelfAlignmentChanged();
  void SchedulePaintForAllButtons();

  // Returns the ideal bounds of the specified item, or an empty rect if id
  // isn't know.
  gfx::Rect GetIdealBoundsOfItemIcon(LauncherID id);

  // Repositions the icon for the specified item by the midpoint of the window.
  void UpdatePanelIconPosition(LauncherID id, const gfx::Point& midpoint);

  void AddIconObserver(LauncherIconObserver* observer);
  void RemoveIconObserver(LauncherIconObserver* observer);

  // Returns true if we're showing a menu.
  bool IsShowingMenu() const;

  // Returns true if overflow bubble is shown.
  bool IsShowingOverflowBubble() const;

  views::View* GetAppListButtonView() const;

  // Returns true if the mouse cursor exits the area for launcher tooltip.
  // There are thin gaps between launcher buttons but the tooltip shouldn't hide
  // in the gaps, but the tooltip should hide if the mouse moved totally outside
  // of the buttons area.
  bool ShouldHideTooltip(const gfx::Point& cursor_location);

  int leading_inset() const { return leading_inset_; }
  void set_leading_inset(int leading_inset) { leading_inset_ = leading_inset; }

  // Overridden from FocusTraversable:
  virtual views::FocusSearch* GetFocusSearch() OVERRIDE;
  virtual FocusTraversable* GetFocusTraversableParent() OVERRIDE;
  virtual View* GetFocusTraversableParentView() OVERRIDE;

  // Overridden from app_list::ApplicationDragAndDropHost:
  virtual void CreateDragIconProxy(
      const gfx::Point& location_in_screen_coordinates,
      const gfx::ImageSkia& icon,
      views::View* replaced_view,
      const gfx::Vector2d& cursor_offset_from_center,
      float scale_factor) OVERRIDE;
  virtual void UpdateDragIconProxy(
      const gfx::Point& location_in_screen_coordinates) OVERRIDE;
  virtual void DestroyDragIconProxy() OVERRIDE;
  virtual bool StartDrag(
      const std::string& app_id,
      const gfx::Point& location_in_screen_coordinates) OVERRIDE;
  virtual bool Drag(const gfx::Point& location_in_screen_coordinates) OVERRIDE;
  virtual void EndDrag(bool cancel) OVERRIDE;

  // Return the view model for test purposes.
  const views::ViewModel* const view_model_for_test() const {
    return view_model_.get();
  }

 private:
  friend class ash::test::LauncherViewTestAPI;

  class FadeOutAnimationDelegate;
  class StartFadeAnimationDelegate;

  struct IdealBounds {
    gfx::Rect overflow_bounds;
  };

  bool is_overflow_mode() const {
    return first_visible_index_ > 0;
  }

  bool dragging() const {
    return drag_pointer_ != NONE;
  }

  // Sets the bounds of each view to its ideal bounds.
  void LayoutToIdealBounds();

  // Calculates the ideal bounds. The bounds of each button corresponding to an
  // item in the model is set in |view_model_|.
  void CalculateIdealBounds(IdealBounds* bounds);

  // Returns the index of the last view whose max primary axis coordinate is
  // less than |max_value|. Returns -1 if nothing fits, or there are no views.
  int DetermineLastVisibleIndex(int max_value) const;

  // Returns the index of the first panel whose min primary axis coordinate is
  // at least |min_value|. Returns the index past the last panel if none fit.
  int DetermineFirstVisiblePanelIndex(int min_value) const;

  // Animates the bounds of each view to its ideal bounds.
  void AnimateToIdealBounds();

  // Creates the view used to represent |item|.
  views::View* CreateViewForItem(const LauncherItem& item);

  // Fades |view| from an opacity of 0 to 1. This is when adding a new item.
  void FadeIn(views::View* view);

  // Invoked when the pointer has moved enough to trigger a drag. Sets
  // internal state in preparation for the drag.
  void PrepareForDrag(Pointer pointer, const ui::LocatedEvent& event);

  // Invoked when the mouse is dragged. Updates the models as appropriate.
  void ContinueDrag(const ui::LocatedEvent& event);

  // Handles ripping off an item from the shelf. Returns true when the item got
  // removed.
  bool HandleRipOffDrag(const ui::LocatedEvent& event);

  // Finalize the rip off dragging by either |cancel| the action or validating.
  void FinalizeRipOffDrag(bool cancel);

  // Returns true if |typea| and |typeb| should be in the same drag range.
  bool SameDragType(LauncherItemType typea, LauncherItemType typeb) const;

  // Returns the range (in the model) the item at the specified index can be
  // dragged to.
  std::pair<int, int> GetDragRange(int index);

  // If there is a drag operation in progress it's canceled. If |modified_index|
  // is valid, the new position of the corresponding item is returned.
  int CancelDrag(int modified_index);

  // Common setup done for all children.
  void ConfigureChildView(views::View* view);

  // Toggles the overflow menu.
  void ToggleOverflowBubble();

  // Update first launcher button's padding. This method adds padding to the
  // first button to include the leading inset. It needs to be called once on
  // button creation and every time when shelf alignment is changed.
  void UpdateFirstButtonPadding();

  // Invoked after the fading out animation for item deletion is ended.
  void OnFadeOutAnimationEnded();

  // Updates the visible range of overflow items in |overflow_view|.
  void UpdateOverflowRange(LauncherView* overflow_view);

  // Overridden from views::View:
  virtual gfx::Size GetPreferredSize() OVERRIDE;
  virtual void OnBoundsChanged(const gfx::Rect& previous_bounds) OVERRIDE;
  virtual FocusTraversable* GetPaneFocusTraversable() OVERRIDE;
  virtual void GetAccessibleState(ui::AccessibleViewState* state) OVERRIDE;

  // Overridden from ui::EventHandler:
  virtual void OnGestureEvent(ui::GestureEvent* event) OVERRIDE;

  // Overridden from LauncherModelObserver:
  virtual void LauncherItemAdded(int model_index) OVERRIDE;
  virtual void LauncherItemRemoved(int model_index, LauncherID id) OVERRIDE;
  virtual void LauncherItemChanged(int model_index,
                                   const ash::LauncherItem& old_item) OVERRIDE;
  virtual void LauncherItemMoved(int start_index, int target_index) OVERRIDE;
  virtual void LauncherStatusChanged() OVERRIDE;

  // Overridden from LauncherButtonHost:
  virtual void PointerPressedOnButton(
      views::View* view,
      Pointer pointer,
      const ui::LocatedEvent& event) OVERRIDE;
  virtual void PointerDraggedOnButton(
      views::View* view,
      Pointer pointer,
      const ui::LocatedEvent& event) OVERRIDE;
  virtual void PointerReleasedOnButton(views::View* view,
                                       Pointer pointer,
                                       bool canceled) OVERRIDE;
  virtual void MouseMovedOverButton(views::View* view) OVERRIDE;
  virtual void MouseEnteredButton(views::View* view) OVERRIDE;
  virtual void MouseExitedButton(views::View* view) OVERRIDE;
  virtual base::string16 GetAccessibleName(const views::View* view) OVERRIDE;

  // Overridden from views::ButtonListener:
  virtual void ButtonPressed(views::Button* sender,
                             const ui::Event& event) OVERRIDE;

  // Show the list of all running items for this |item|. It will return true
  // when the menu was shown and false if there were no possible items to
  // choose from. |source| specifies the view which is responsible for showing
  // the menu, and the bubble will point towards it.
  // The |event_flags| are the flags of the event which triggered this menu.
  bool ShowListMenuForView(const LauncherItem& item,
                           views::View* source,
                           const ui::Event& event);

  // Overridden from views::ContextMenuController:
  virtual void ShowContextMenuForView(views::View* source,
                                      const gfx::Point& point,
                                      ui::MenuSourceType source_type) OVERRIDE;

  // Show either a context or normal click menu of given |menu_model_adapter|.
  // If |context_menu| is set, the displayed menu is a context menu and not
  // a menu listing one or more running applications.
  // The |click_point| is only used for |context_menu|'s.
  void ShowMenu(scoped_ptr<views::MenuModelAdapter> menu_model_adapter,
                views::View* source,
                const gfx::Point& click_point,
                bool context_menu,
                ui::MenuSourceType source_type);

  // Overridden from views::BoundsAnimatorObserver:
  virtual void OnBoundsAnimatorProgressed(
      views::BoundsAnimator* animator) OVERRIDE;
  virtual void OnBoundsAnimatorDone(views::BoundsAnimator* animator) OVERRIDE;

  // Returns false if the click which closed the previous menu is the click
  // which triggered this event.
  bool IsUsableEvent(const ui::Event& event);

  // Convenience accessor to model_->items().
  const LauncherItem* LauncherItemForView(const views::View* view) const;

  // Returns true if a tooltip should be shown for |view|.
  bool ShouldShowTooltipForView(const views::View* view) const;

  // Get the distance from the given |coordinate| to the closest point on this
  // launcher/shelf.
  int CalculateShelfDistance(const gfx::Point& coordinate) const;

  // The model; owned by Launcher.
  LauncherModel* model_;

  // Delegate; owned by Launcher.
  LauncherDelegate* delegate_;

  // Used to manage the set of active launcher buttons. There is a view per
  // item in |model_|.
  scoped_ptr<views::ViewModel> view_model_;

  // Index of first visible launcher item. When it it greater than 0,
  // LauncherView is hosted in an overflow bubble. In this mode, it does not
  // show browser, app list and overflow button.
  int first_visible_index_;

  // Last index of a launcher button that is visible
  // (does not go into overflow).
  int last_visible_index_;

  scoped_ptr<views::BoundsAnimator> bounds_animator_;

  OverflowButton* overflow_button_;

  scoped_ptr<OverflowBubble> overflow_bubble_;

  scoped_ptr<LauncherTooltipManager> tooltip_;

  // Pointer device that initiated the current drag operation. If there is no
  // current dragging operation, this is NONE.
  Pointer drag_pointer_;

  // The view being dragged. This is set immediately when the mouse is pressed.
  // |dragging_| is set only if the mouse is dragged far enough.
  views::View* drag_view_;

  // X coordinate of the mouse down event in |drag_view_|s coordinates.
  int drag_offset_;

  // Index |drag_view_| was initially at.
  int start_drag_index_;

  // Used for the context menu of a particular item.
  LauncherID context_menu_id_;

  scoped_ptr<views::FocusSearch> focus_search_;

#if !defined(OS_MACOSX)
  scoped_ptr<views::MenuRunner> launcher_menu_runner_;
#endif

  ObserverList<LauncherIconObserver> observers_;

  // Amount content is inset on the left edge (or top edge for vertical
  // alignment).
  int leading_inset_;

  ShelfGestureHandler gesture_handler_;

  // True when an item being inserted or removed in the model cancels a drag.
  bool cancelling_drag_model_changed_;

  // Index of the last hidden launcher item. If there are no hidden items this
  // will be equal to last_visible_index_ + 1.
  int last_hidden_index_;

  // The timestamp of the event which closed the last menu - or 0.
  base::TimeDelta closing_event_time_;

  // When this object gets deleted while a menu is shown, this pointed
  // element will be set to false.
  bool* got_deleted_;

  // True if a drag and drop operation created/pinned the item in the launcher
  // and it needs to be deleted/unpinned again if the operation gets cancelled.
  bool drag_and_drop_item_pinned_;

  // The launcher item which is currently used for a drag and a drop operation
  // or 0 otherwise.
  LauncherID drag_and_drop_launcher_id_;

  // The application ID of the application which we drag and drop.
  std::string drag_and_drop_app_id_;

  // The original launcher item's size before the dragging operation.
  gfx::Size pre_drag_and_drop_size_;

  // The image proxy for drag operations when a drag and drop host exists and
  // the item can be dragged outside the app grid.
  scoped_ptr<ash::internal::DragImageView> drag_image_;

  // The cursor offset to the middle of the dragged item.
  gfx::Vector2d drag_image_offset_;

  // The view which gets replaced by our drag icon proxy.
  views::View* drag_replaced_view_;

  // True when the icon was dragged off the shelf.
  bool dragged_off_shelf_;

  // Holds LauncherItemDelegateManager.
  LauncherItemDelegateManager* item_manager_;

  DISALLOW_COPY_AND_ASSIGN(LauncherView);
};

}  // namespace internal
}  // namespace ash

#endif  // ASH_LAUNCHER_LAUNCHER_VIEW_H_
