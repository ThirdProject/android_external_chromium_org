// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "app/l10n_util_mac.h"
#include "base/scoped_nsobject.h"
#include "base/scoped_nsautorelease_pool.h"
#include "base/scoped_ptr.h"
#include "chrome/app/chrome_dll_resource.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/browser_window.h"
#include "chrome/browser/cocoa/browser_test_helper.h"
#include "chrome/browser/cocoa/browser_window_controller.h"
#include "chrome/browser/cocoa/cocoa_test_helper.h"
#include "chrome/browser/cocoa/find_bar_bridge.h"
#include "chrome/browser/sync/sync_status_ui_helper.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/pref_service.h"
#include "chrome/test/testing_browser_process.h"
#include "chrome/test/testing_profile.h"
#include "grit/generated_resources.h"

@interface BrowserWindowController (JustForTesting)
// Already defined in BWC.
- (void)saveWindowPositionToPrefs:(PrefService*)prefs;
- (void)layoutSubviews;
@end

@interface BrowserWindowController (ExposedForTesting)
// Implementations are below.
- (NSView*)infoBarContainerView;
- (NSView*)toolbarView;
- (NSView*)bookmarkView;
- (BOOL)bookmarkBarVisible;
- (NSView*)extensionShelfView;
@end

@implementation BrowserWindowController (ExposedForTesting)
- (NSView*)infoBarContainerView {
  return [infoBarContainerController_ view];
}

- (NSView*)toolbarView {
  return [toolbarController_ view];
}

- (NSView*)bookmarkView {
  return [bookmarkBarController_ view];
}

- (NSView*)findBarView {
  return [findBarCocoaController_ view];
}

- (BOOL)bookmarkBarVisible {
  return [bookmarkBarController_ isVisible];
}

- (NSView*)extensionShelfView {
  return [extensionShelfController_ view];
}
@end

class BrowserWindowControllerTest : public CocoaTest {
 public:
  virtual void SetUp() {
    CocoaTest::SetUp();
    Browser* browser = browser_helper_.browser();
    controller_ = [[BrowserWindowController alloc] initWithBrowser:browser
                                                     takeOwnership:NO];
  }

  virtual void TearDown() {
    [controller_ close];
    CocoaTest::TearDown();
  }

 public:
  BrowserTestHelper browser_helper_;
  BrowserWindowController* controller_;
};

TEST_F(BrowserWindowControllerTest, TestSaveWindowPosition) {
  PrefService* prefs = browser_helper_.profile()->GetPrefs();
  ASSERT_TRUE(prefs != NULL);

  // Check to make sure there is no existing pref for window placement.
  ASSERT_TRUE(prefs->GetDictionary(prefs::kBrowserWindowPlacement) == NULL);

  // Ask the window to save its position, then check that a preference
  // exists.  We're technically passing in a pointer to the user prefs
  // and not the local state prefs, but a PrefService* is a
  // PrefService*, and this is a unittest.
  [controller_ saveWindowPositionToPrefs:prefs];
  EXPECT_TRUE(prefs->GetDictionary(prefs::kBrowserWindowPlacement) != NULL);
}

TEST_F(BrowserWindowControllerTest, TestFullScreenWindow) {
  // Confirm the fullscreen command doesn't return nil.
  // See BrowserWindowFullScreenControllerTest for more fullscreen tests.
  EXPECT_TRUE([controller_ fullscreenWindow]);
}

TEST_F(BrowserWindowControllerTest, TestNormal) {
  // Force the bookmark bar to be shown.
  browser_helper_.profile()->GetPrefs()->
      SetBoolean(prefs::kShowBookmarkBar, true);

  // Make sure a normal BrowserWindowController is, uh, normal.
  EXPECT_TRUE([controller_ isNormalWindow]);
  EXPECT_TRUE([controller_ isBookmarkBarVisible]);

  // And make sure a controller for a pop-up window is not normal.
  // popup_browser will be owned by its window.
  Browser *popup_browser(Browser::CreateForPopup(browser_helper_.profile()));
  NSWindow *cocoaWindow = popup_browser->window()->GetNativeHandle();
  BrowserWindowController* controller =
      static_cast<BrowserWindowController*>([cocoaWindow windowController]);
  ASSERT_TRUE([controller isKindOfClass:[BrowserWindowController class]]);
  EXPECT_FALSE([controller isNormalWindow]);
  EXPECT_FALSE([controller isBookmarkBarVisible]);
  [controller close];
}

@interface GTMTheme (BrowserThemeProviderInitialization)
+ (GTMTheme *)themeWithBrowserThemeProvider:(BrowserThemeProvider *)provider
                             isOffTheRecord:(BOOL)isOffTheRecord;
@end

TEST_F(BrowserWindowControllerTest, TestTheme) {
  [controller_ userChangedTheme];
}

TEST_F(BrowserWindowControllerTest, BookmarkBarControllerIndirection) {
  EXPECT_FALSE([controller_ isBookmarkBarVisible]);

  // Explicitly show the bar. Can't use bookmark_utils::ToggleWhenVisible()
  // because of the notification issues.
  browser_helper_.profile()->GetPrefs()->
      SetBoolean(prefs::kShowBookmarkBar, true);

  [controller_ updateBookmarkBarVisibility];
  EXPECT_TRUE([controller_ isBookmarkBarVisible]);
}

#if 0
// TODO(jrg): This crashes trying to create the BookmarkBarController, adding
// an observer to the BookmarkModel.
TEST_F(BrowserWindowControllerTest, TestIncognitoWidthSpace) {
  scoped_ptr<TestingProfile> incognito_profile(new TestingProfile());
  incognito_profile->set_off_the_record(true);
  scoped_ptr<Browser> browser(new Browser(Browser::TYPE_NORMAL,
                                          incognito_profile.get()));
  controller_.reset([[BrowserWindowController alloc]
                              initWithBrowser:browser.get()
                                takeOwnership:NO]);

  NSRect tabFrame = [[controller_ tabStripView] frame];
  [controller_ installIncognitoBadge];
  NSRect newTabFrame = [[controller_ tabStripView] frame];
  EXPECT_GT(tabFrame.size.width, newTabFrame.size.width);

  controller_.release();
}
#endif

namespace {
// Verifies that the toolbar, infobar, tab content area, download shelf, and
// extension shelf completely fill their window's contentView.
void CheckViewPositions(BrowserWindowController* controller) {
  NSRect contentView = [[[controller window] contentView] bounds];
  NSRect toolbar = [[controller toolbarView] frame];
  NSRect infobar = [[controller infoBarContainerView] frame];
  NSRect contentArea = [[controller tabContentArea] frame];
  NSRect download = [[[controller downloadShelf] view] frame];
  NSRect extension = [[controller extensionShelfView] frame];

  EXPECT_EQ(NSMinY(contentView), NSMinY(extension));
  EXPECT_EQ(NSMaxY(extension), NSMinY(download));
  EXPECT_EQ(NSMaxY(download), NSMinY(contentArea));
  EXPECT_EQ(NSMaxY(contentArea), NSMinY(infobar));

  // Bookmark bar frame is random memory when hidden.
  if ([controller bookmarkBarVisible]) {
    NSRect bookmark = [[controller bookmarkView] frame];
    EXPECT_EQ(NSMaxY(infobar), NSMinY(bookmark));
    EXPECT_EQ(NSMaxY(bookmark), NSMinY(toolbar));
  } else {
    EXPECT_EQ(NSMaxY(infobar), NSMinY(toolbar));
  }

  EXPECT_EQ(NSMaxY(contentView), NSMaxY(toolbar));
}
}  // end namespace

// Test to make sure resizing and relaying-out subviews works correctly.
TEST_F(BrowserWindowControllerTest, TestResizeViews) {
  TabStripView* tabstrip = [controller_ tabStripView];
  NSView* contentView = [[tabstrip window] contentView];
  NSView* toolbar = [controller_ toolbarView];
  NSView* infobar = [controller_ infoBarContainerView];
  NSView* extensionShelf = [controller_ extensionShelfView];

  // We need to muck with the views a bit to put us in a consistent state before
  // we start resizing.  In particular, we need to move the tab strip to be
  // immediately above the content area, since we layout views to be directly
  // under the tab strip.  We also explicitly set the contentView's frame to be
  // 800x600.
  [contentView setFrame:NSMakeRect(0, 0, 800, 600)];
  NSRect tabstripFrame = [tabstrip frame];
  tabstripFrame.origin.y = NSMaxY([contentView frame]);
  [tabstrip setFrame:tabstripFrame];

  // The download shelf is created lazily.  Force-create it and set its initial
  // height to 0.
  NSView* download = [[controller_ downloadShelf] view];
  [controller_ resizeView:download newHeight:0];

  // Force a layout and check each view's frame.
  [controller_ layoutSubviews];
  CheckViewPositions(controller_);

  // Add an extension shelf and recheck.
  [controller_ resizeView:extensionShelf newHeight:40];
  CheckViewPositions(controller_);

  // Expand the infobar to 60px and recheck
  [controller_ resizeView:infobar newHeight:60];
  CheckViewPositions(controller_);

  // Expand the toolbar to 64px and recheck
  [controller_ resizeView:toolbar newHeight:64];
  CheckViewPositions(controller_);

  // Add a 30px download shelf and recheck
  [controller_ resizeView:download newHeight:30];
  CheckViewPositions(controller_);

  // Shrink the infobar to 0px and toolbar to 39px and recheck
  [controller_ resizeView:infobar newHeight:0];
  [controller_ resizeView:toolbar newHeight:39];
  CheckViewPositions(controller_);
}

TEST_F(BrowserWindowControllerTest, TestResizeViewsWithBookmarkBar) {
  // Force a display of the bookmark bar.
  browser_helper_.profile()->GetPrefs()->
      SetBoolean(prefs::kShowBookmarkBar, true);

  TabStripView* tabstrip = [controller_ tabStripView];
  NSView* contentView = [[tabstrip window] contentView];
  NSView* toolbar = [controller_ toolbarView];
  NSView* bookmark = [controller_ bookmarkView];
  NSView* infobar = [controller_ infoBarContainerView];
  NSView* extensionShelf = [controller_ extensionShelfView];

  // We need to muck with the views a bit to put us in a consistent state before
  // we start resizing.  In particular, we need to move the tab strip to be
  // immediately above the content area, since we layout views to be directly
  // under the tab strip.  We also explicitly set the contentView's frame to be
  // 800x600.
  [contentView setFrame:NSMakeRect(0, 0, 800, 600)];
  NSRect tabstripFrame = [tabstrip frame];
  tabstripFrame.origin.y = NSMaxY([contentView frame]);
  [tabstrip setFrame:tabstripFrame];

  // The download shelf is created lazily.  Force-create it and set its initial
  // height to 0.
  NSView* download = [[controller_ downloadShelf] view];
  [controller_ resizeView:download newHeight:0];

  // Force a layout and check each view's frame.
  [controller_ layoutSubviews];
  CheckViewPositions(controller_);

  // Add the bookmark bar and recheck.
  [controller_ resizeView:bookmark newHeight:40];
  CheckViewPositions(controller_);

  // Add an extension shelf and recheck.
  [controller_ resizeView:extensionShelf newHeight:40];
  CheckViewPositions(controller_);

  // Expand the infobar to 60px and recheck
  [controller_ resizeView:infobar newHeight:60];
  CheckViewPositions(controller_);

  // Expand the toolbar to 64px and recheck
  [controller_ resizeView:toolbar newHeight:64];
  CheckViewPositions(controller_);

  // Add a 30px download shelf and recheck
  [controller_ resizeView:download newHeight:30];
  CheckViewPositions(controller_);

  // Remove the bookmark bar and recheck
  browser_helper_.profile()->GetPrefs()->
      SetBoolean(prefs::kShowBookmarkBar, false);
  [controller_ resizeView:bookmark newHeight:0];
  CheckViewPositions(controller_);

  // Shrink the infobar to 0px and toolbar to 39px and recheck
  [controller_ resizeView:infobar newHeight:0];
  [controller_ resizeView:toolbar newHeight:39];
  CheckViewPositions(controller_);
}

// Make sure, by default, the bookmark bar and the toolbar are the same width.
TEST_F(BrowserWindowControllerTest, BookmarkBarIsSameWidth) {
  // Set the pref to the bookmark bar is visible when the toolbar is
  // first created.
  browser_helper_.profile()->GetPrefs()->SetBoolean(
      prefs::kShowBookmarkBar, true);

  // Make sure the bookmark bar is the same width as the toolbar
  NSView* bookmarkBarView = [controller_ bookmarkView];
  NSView* toolbarView = [controller_ toolbarView];
  EXPECT_EQ([toolbarView frame].size.width,
            [bookmarkBarView frame].size.width);
}

TEST_F(BrowserWindowControllerTest, TestTopLeftForBubble) {
  NSPoint p = [controller_ topLeftForBubble];
  NSRect all = [[controller_ window] frame];

  // As a sanity check make sure the point is vaguely in the top left
  // of the window.
  EXPECT_GT(p.y, all.origin.y + (all.size.height/2));
  EXPECT_LT(p.x, all.origin.x + (all.size.width/2));
}

// By the "zoom frame", we mean what Apple calls the "standard frame".
TEST_F(BrowserWindowControllerTest, TestZoomFrame) {
  NSWindow* window = [controller_ window];
  ASSERT_TRUE(window);
  NSRect screenFrame = [[window screen] visibleFrame];
  ASSERT_FALSE(NSIsEmptyRect(screenFrame));

  // Minimum zoomed width is the larger of 60% of available horizontal space or
  // 60% of available vertical space, subject to available horizontal space.
  CGFloat minZoomWidth =
      std::min(std::max((CGFloat)0.6 * screenFrame.size.width,
                        (CGFloat)0.6 * screenFrame.size.height),
               screenFrame.size.width);

  // |testFrame| is the size of the window we start out with, and |zoomFrame| is
  // the one returned by |-windowWillUseStandardFrame:defaultFrame:|.
  NSRect testFrame;
  NSRect zoomFrame;

  // 1. Test a case where it zooms the window both horizontally and vertically,
  // and only moves it vertically. "+ 32", etc. are just arbitrary constants
  // used to check that the window is moved properly and not just to the origin;
  // they should be small enough to not shove windows off the screen.
  testFrame.size.width = 0.5 * minZoomWidth;
  testFrame.size.height = 0.5 * screenFrame.size.height;
  testFrame.origin.x = screenFrame.origin.x + 32;  // See above.
  testFrame.origin.y = screenFrame.origin.y + 23;
  [window setFrame:testFrame display:NO];
  zoomFrame = [controller_ windowWillUseStandardFrame:window
                                         defaultFrame:screenFrame];
  EXPECT_LE(minZoomWidth, zoomFrame.size.width);
  EXPECT_EQ(screenFrame.size.height, zoomFrame.size.height);
  EXPECT_EQ(testFrame.origin.x, zoomFrame.origin.x);
  EXPECT_EQ(screenFrame.origin.y, zoomFrame.origin.y);

  // 2. Test a case where it zooms the window only horizontally, and only moves
  // it horizontally.
  testFrame.size.width = 0.5 * minZoomWidth;
  testFrame.size.height = screenFrame.size.height;
  testFrame.origin.x = screenFrame.origin.x + screenFrame.size.width -
                       testFrame.size.width;
  testFrame.origin.y = screenFrame.origin.y;
  [window setFrame:testFrame display:NO];
  zoomFrame = [controller_ windowWillUseStandardFrame:window
                                         defaultFrame:screenFrame];
  EXPECT_LE(minZoomWidth, zoomFrame.size.width);
  EXPECT_EQ(screenFrame.size.height, zoomFrame.size.height);
  EXPECT_EQ(screenFrame.origin.x + screenFrame.size.width -
            zoomFrame.size.width, zoomFrame.origin.x);
  EXPECT_EQ(screenFrame.origin.y, zoomFrame.origin.y);

  // 3. Test a case where it zooms the window only vertically, and only moves it
  // vertically.
  testFrame.size.width = std::min((CGFloat)1.1 * minZoomWidth,
                                  screenFrame.size.width);
  testFrame.size.height = 0.3 * screenFrame.size.height;
  testFrame.origin.x = screenFrame.origin.x + 32;  // See above (in 1.).
  testFrame.origin.y = screenFrame.origin.y + 123;
  [window setFrame:testFrame display:NO];
  zoomFrame = [controller_ windowWillUseStandardFrame:window
                                         defaultFrame:screenFrame];
  // Use the actual width of the window frame, since it's subject to rounding.
  EXPECT_EQ([window frame].size.width, zoomFrame.size.width);
  EXPECT_EQ(screenFrame.size.height, zoomFrame.size.height);
  EXPECT_EQ(testFrame.origin.x, zoomFrame.origin.x);
  EXPECT_EQ(screenFrame.origin.y, zoomFrame.origin.y);

  // 4. Test a case where zooming should do nothing (i.e., we're already at a
  // zoomed frame).
  testFrame.size.width = std::min((CGFloat)1.1 * minZoomWidth,
                                  screenFrame.size.width);
  testFrame.size.height = screenFrame.size.height;
  testFrame.origin.x = screenFrame.origin.x + 32;  // See above (in 1.).
  testFrame.origin.y = screenFrame.origin.y;
  [window setFrame:testFrame display:NO];
  zoomFrame = [controller_ windowWillUseStandardFrame:window
                                         defaultFrame:screenFrame];
  // Use the actual width of the window frame, since it's subject to rounding.
  EXPECT_EQ([window frame].size.width, zoomFrame.size.width);
  EXPECT_EQ(screenFrame.size.height, zoomFrame.size.height);
  EXPECT_EQ(testFrame.origin.x, zoomFrame.origin.x);
  EXPECT_EQ(screenFrame.origin.y, zoomFrame.origin.y);
}

TEST_F(BrowserWindowControllerTest, TestFindBarOnTop) {
  FindBarBridge bridge;
  [controller_ addFindBar:bridge.find_bar_cocoa_controller()];

  // Test that the Z-order of the find bar is on top of everything.
  NSArray* subviews = [[[controller_ window] contentView] subviews];
  NSUInteger findBar_index =
      [subviews indexOfObject:[controller_ findBarView]];
  EXPECT_NE(NSNotFound, findBar_index);
  NSUInteger toolbar_index =
      [subviews indexOfObject:[controller_ toolbarView]];
  EXPECT_NE(NSNotFound, toolbar_index);
  NSUInteger bookmark_index =
      [subviews indexOfObject:[controller_ bookmarkView]];
  EXPECT_NE(NSNotFound, bookmark_index);

  EXPECT_GT(findBar_index, toolbar_index);
  EXPECT_GT(findBar_index, bookmark_index);
}

TEST_F(BrowserWindowControllerTest, TestSyncMenuItem) {
  scoped_nsobject<NSMenuItem> syncMenuItem(
      [[NSMenuItem alloc] initWithTitle:@""
                                 action:@selector(commandDispatch)
                          keyEquivalent:@""]);
  [syncMenuItem setTag:IDC_SYNC_BOOKMARKS];

  NSString* bookmarksSynced =
      l10n_util::GetNSStringWithFixup(IDS_SYNC_MENU_BOOKMARKS_SYNCED_LABEL);
  NSString* bookmarkSyncError =
      l10n_util::GetNSStringWithFixup(IDS_SYNC_MENU_BOOKMARK_SYNC_ERROR_LABEL);
  NSString* startSync =
      l10n_util::GetNSStringWithFixup(IDS_SYNC_START_SYNC_BUTTON_LABEL);

  [syncMenuItem setTitle:@""];
  [syncMenuItem setHidden:NO];
  [controller_ updateSyncItem:syncMenuItem
                  syncEnabled:NO
                       status:SyncStatusUIHelper::PRE_SYNCED];
  EXPECT_TRUE([[syncMenuItem title] isEqualTo:startSync]);
  EXPECT_TRUE([syncMenuItem isHidden]);

  [syncMenuItem setTitle:@""];
  [syncMenuItem setHidden:YES];
  [controller_ updateSyncItem:syncMenuItem
                  syncEnabled:YES
                       status:SyncStatusUIHelper::SYNC_ERROR];
  EXPECT_TRUE([[syncMenuItem title] isEqualTo:bookmarkSyncError]);
  EXPECT_FALSE([syncMenuItem isHidden]);

  [syncMenuItem setTitle:@""];
  [syncMenuItem setHidden:NO];
  [controller_ updateSyncItem:syncMenuItem
                  syncEnabled:NO
                       status:SyncStatusUIHelper::SYNCED];
  EXPECT_TRUE([[syncMenuItem title] isEqualTo:bookmarksSynced]);
  EXPECT_TRUE([syncMenuItem isHidden]);
}

TEST_F(BrowserWindowControllerTest, TestSyncMenuItemWithSeparator) {
  scoped_nsobject<NSMenu> menu([[NSMenu alloc] initWithTitle:@""]);
  NSMenuItem* syncMenuItem =
      [menu addItemWithTitle:@""
                      action:@selector(commandDispatch)
               keyEquivalent:@""];
  [syncMenuItem setTag:IDC_SYNC_BOOKMARKS];
  NSMenuItem* following_separator = [NSMenuItem separatorItem];
  [menu addItem:following_separator];

  const SyncStatusUIHelper::MessageType kStatus =
      SyncStatusUIHelper::PRE_SYNCED;

  [syncMenuItem setHidden:NO];
  [following_separator setHidden:NO];
  [controller_ updateSyncItem:syncMenuItem
                  syncEnabled:NO
                       status:kStatus];
  EXPECT_FALSE([following_separator isEnabled]);
  EXPECT_TRUE([syncMenuItem isHidden]);
  EXPECT_TRUE([following_separator isHidden]);

  [syncMenuItem setHidden:YES];
  [following_separator setHidden:YES];
  [controller_ updateSyncItem:syncMenuItem
                  syncEnabled:YES
                       status:kStatus];
  EXPECT_FALSE([following_separator isEnabled]);
  EXPECT_FALSE([syncMenuItem isHidden]);
  EXPECT_FALSE([following_separator isHidden]);
}

@interface BrowserWindowControllerFakeFullscreen : BrowserWindowController {
 @private
  // We release the window ourselves, so we don't have to rely on the unittest
  // doing it for us.
  scoped_nsobject<NSWindow> fullscreenWindow_;
}
@end

class BrowserWindowFullScreenControllerTest : public CocoaTest {
 public:
  virtual void SetUp() {
    CocoaTest::SetUp();
    Browser* browser = browser_helper_.browser();
    controller_ =
        [[BrowserWindowControllerFakeFullscreen alloc] initWithBrowser:browser
                                                         takeOwnership:NO];
  }

  virtual void TearDown() {
    [controller_ close];
    CocoaTest::TearDown();
  }

 public:
  BrowserTestHelper browser_helper_;
  BrowserWindowController* controller_;
};

@interface BrowserWindowController (PrivateAPI)
- (BOOL)supportsFullscreen;
@end

// Fullscreen mode disabled for Mstone-4 / ReleaseBlock-Beta.
// Confirm we don't accidentally turn it back on.
TEST_F(BrowserWindowFullScreenControllerTest, ConfirmFullscreenDisabled) {
  EXPECT_FALSE([controller_ supportsFullscreen]);
}

TEST_F(BrowserWindowFullScreenControllerTest, DISABLED_TestFullscreen) {
  EXPECT_FALSE([controller_ isFullscreen]);
  [controller_ setFullscreen:YES];
  EXPECT_TRUE([controller_ isFullscreen]);
  [controller_ setFullscreen:NO];
  EXPECT_FALSE([controller_ isFullscreen]);
}

TEST_F(BrowserWindowFullScreenControllerTest, DISABLED_TestActivate) {
  EXPECT_FALSE([controller_ isFullscreen]);

  [controller_ activate];
  NSWindow* frontmostWindow = [[NSApp orderedWindows] objectAtIndex:0];
  EXPECT_EQ(frontmostWindow, [controller_ window]);

  [controller_ setFullscreen:YES];
  [controller_ activate];
  frontmostWindow = [[NSApp orderedWindows] objectAtIndex:0];
  EXPECT_EQ(frontmostWindow, [controller_ fullscreenWindow]);

  // We have to cleanup after ourselves by unfullscreening.
  [controller_ setFullscreen:NO];
}

@implementation BrowserWindowControllerFakeFullscreen
// Override fullscreenWindow to return a dummy window.  This isn't needed to
// pass the test, but because the dummy window is only 100x100, it prevents the
// real fullscreen window from flashing up and taking over the whole screen..
// We have to return an actual window because layoutSubviews: looks at the
// window's frame.
- (NSWindow*)fullscreenWindow {
  if (fullscreenWindow_.get())
    return fullscreenWindow_.get();

  fullscreenWindow_.reset(
      [[NSWindow alloc] initWithContentRect:NSMakeRect(0,0,400,400)
                                  styleMask:NSBorderlessWindowMask
                                    backing:NSBackingStoreBuffered
                                      defer:NO]);
  return fullscreenWindow_.get();
}
@end

/* TODO(???): test other methods of BrowserWindowController */
