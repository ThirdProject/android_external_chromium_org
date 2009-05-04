// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DEBUGGER_DEVTOOLS_VIEW_H_
#define CHROME_BROWSER_DEBUGGER_DEVTOOLS_VIEW_H_

#include <string>

#include "base/basictypes.h"
#include "base/gfx/size.h"
#include "chrome/browser/tab_contents/tab_contents_delegate.h"
#include "chrome/views/view.h"

namespace IPC {
class Message;
}

class RenderViewHost;
class TabContentsContainerView;
class TabContents;

class DevToolsView : public views::View,
                     public TabContentsDelegate {
 public:
  explicit DevToolsView();
  virtual ~DevToolsView();

  // Destroy content views when the window is closing.
  void OnWindowClosing();
  void SendMessageToClient(const IPC::Message& message);
  bool HasRenderViewHost(const RenderViewHost& rvh) const;

 private:
  // Overridden from TabContentsDelegate:
  virtual void NavigationStateChanged(const TabContents* source,
                                      unsigned changed_flags) {}
  virtual void AddNewContents(TabContents* source,
                              TabContents* new_contents,
                              WindowOpenDisposition disposition,
                              const gfx::Rect& initial_pos,
                              bool user_gesture) {}
  virtual void ActivateContents(TabContents* contents) {}
  virtual void LoadingStateChanged(TabContents* source) {}
  virtual void CloseContents(TabContents* source) {}
  virtual void MoveContents(TabContents* source, const gfx::Rect& pos) {}
  virtual bool IsPopup(TabContents* source) { return false; }
  virtual void ToolbarSizeChanged(TabContents* source, bool is_animating) {}
  virtual void URLStarredChanged(TabContents* source, bool) {}
  virtual void UpdateTargetURL(TabContents* source, const GURL& url) {}
  virtual bool CanBlur() const { return false; }

  // Opens a new URL inside the passed in TabContents, if source is 0 open
  // in the current front-most tab.
  virtual void OpenURLFromTab(TabContents* source,
                              const GURL& url, const GURL& referrer,
                              WindowOpenDisposition disposition,
                              PageTransition::Type transition);

  // Overridden from views::View:
  virtual std::string GetClassName() const;
  virtual gfx::Size GetPreferredSize();
  virtual void Layout();
  virtual void ViewHierarchyChanged(bool is_add,
                                    views::View* parent,
                                    views::View* child);

  void Init();

  TabContents* tab_contents_;
  TabContentsContainerView* web_container_;

  DISALLOW_COPY_AND_ASSIGN(DevToolsView);
};

#endif  // CHROME_BROWSER_DEBUGGER_DEVTOOLS_VIEW_H_
