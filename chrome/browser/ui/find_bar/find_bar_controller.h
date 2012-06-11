// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_FIND_BAR_FIND_BAR_CONTROLLER_H_
#define CHROME_BROWSER_UI_FIND_BAR_FIND_BAR_CONTROLLER_H_
#pragma once

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

namespace gfx {
class Rect;
}

class FindBar;
class TabContents;

class FindBarController : public content::NotificationObserver {
 public:
  // An enum listing the possible actions to take on a find-in-page selection
  // in the page when ending the find session.
  enum SelectionAction {
    kKeepSelectionOnPage,     // Translate the find selection into a normal
                              // selection.
    kClearSelectionOnPage,    // Clear the find selection.
    kActivateSelectionOnPage  // Focus and click the selected node (for links).
  };

  // An enum listing the possible actions to take on a find-in-page results in
  // the Find box when ending the find session.
  enum ResultAction {
    kClearResultsInFindBox,  // Clear search string, ordinal and match count.
    kKeepResultsInFindBox,   // Leave the results untouched.
  };

  // FindBar takes ownership of |find_bar_view|.
  explicit FindBarController(FindBar* find_bar);

  virtual ~FindBarController();

  // Shows the find bar. Any previous search string will again be visible.
  void Show();

  // Ends the current session. |selection_action| specifies what to do with the
  // selection on the page created by the find operation. |results_action|
  // specifies what to do with the contents of the Find box (after ending).
  void EndFindSession(SelectionAction selection_action,
                      ResultAction results_action);

  // Accessor for the attached TabContents.
  TabContents* tab_contents() const { return tab_contents_; }

  // Changes the TabContents that this FindBar is attached to. This
  // occurs when the user switches tabs in the Browser window. |contents| can be
  // NULL.
  void ChangeTabContents(TabContents* contents);

  // Overridden from content::NotificationObserver:
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE;

  FindBar* find_bar() const { return find_bar_.get(); }

  // Reposition |view_location| such that it avoids |avoid_overlapping_rect|,
  // and return the new location.
  static gfx::Rect GetLocationForFindbarView(
      gfx::Rect view_location,
      const gfx::Rect& dialog_bounds,
      const gfx::Rect& avoid_overlapping_rect);

 private:
  // Sents an update to the find bar with the tab contents' current result. The
  // tab_contents_ must be non-NULL before this call. Theis handles
  // de-flickering in addition to just calling the update function.
  void UpdateFindBarForCurrentResult();

  // For Windows and Linux this function sets the prepopulate text for the
  // Find text box. The propopulate value is the last value the user searched
  // for in the current tab, or (if blank) the last value searched for in any
  // tab. Mac has a global value for search, so this function does nothing on
  // Mac.
  void MaybeSetPrepopulateText();

  content::NotificationRegistrar registrar_;

  scoped_ptr<FindBar> find_bar_;

  // The TabContents we are currently associated with.  Can be NULL.
  TabContents* tab_contents_;

  // The last match count we reported to the user. This is used by
  // UpdateFindBarForCurrentResult to avoid flickering.
  int last_reported_matchcount_;

  DISALLOW_COPY_AND_ASSIGN(FindBarController);
};

#endif  // CHROME_BROWSER_UI_FIND_BAR_FIND_BAR_CONTROLLER_H_
