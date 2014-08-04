// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/website_settings/permission_bubble_controller.h"

#include <algorithm>

#include "base/mac/bind_objc_block.h"
#include "base/mac/foundation_util.h"
#include "base/mac/mac_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#import "chrome/browser/ui/chrome_style.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/browser_window_utils.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_button.h"
#import "chrome/browser/ui/cocoa/hover_close_button.h"
#import "chrome/browser/ui/cocoa/hyperlink_text_view.h"
#import "chrome/browser/ui/cocoa/info_bubble_view.h"
#import "chrome/browser/ui/cocoa/info_bubble_window.h"
#include "chrome/browser/ui/cocoa/website_settings/permission_bubble_cocoa.h"
#include "chrome/browser/ui/cocoa/website_settings/permission_selector_button.h"
#include "chrome/browser/ui/cocoa/website_settings/split_block_button.h"
#include "chrome/browser/ui/cocoa/website_settings/website_settings_utils_cocoa.h"
#include "chrome/browser/ui/website_settings/permission_bubble_request.h"
#include "chrome/browser/ui/website_settings/permission_bubble_view.h"
#include "chrome/browser/ui/website_settings/permission_menu_model.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "content/public/browser/user_metrics.h"
#include "grit/generated_resources.h"
#include "skia/ext/skia_utils_mac.h"
#import "ui/base/cocoa/menu_controller.h"
#include "ui/base/cocoa/window_size_constants.h"
#import "ui/base/cocoa/menu_controller.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/models/simple_menu_model.h"

using base::UserMetricsAction;

namespace {

const CGFloat kHorizontalPadding = 13.0f;
const CGFloat kVerticalPadding = 13.0f;
const CGFloat kBetweenButtonsPadding = 10.0f;
const CGFloat kButtonRightEdgePadding = 17.0f;
const CGFloat kTitlePaddingX = 50.0f;
const CGFloat kBubbleMinWidth = 315.0f;
const NSSize kPermissionIconSize = {18, 18};

class MenuDelegate : public ui::SimpleMenuModel::Delegate {
 public:
  explicit MenuDelegate(PermissionBubbleController* bubble)
      : bubble_controller_(bubble) {}
  virtual bool IsCommandIdChecked(int command_id) const OVERRIDE {
    return false;
  }
  virtual bool IsCommandIdEnabled(int command_id) const OVERRIDE {
    return true;
  }
  virtual bool GetAcceleratorForCommandId(
      int command_id,
      ui::Accelerator* accelerator) OVERRIDE {
    return false;
  }
  virtual void ExecuteCommand(int command_id, int event_flags) OVERRIDE {
    [bubble_controller_ onMenuItemClicked:command_id];
  }
 private:
  PermissionBubbleController* bubble_controller_;  // Weak, owns us.
  DISALLOW_COPY_AND_ASSIGN(MenuDelegate);
};

}  // namespace

// NSPopUpButton with a menu containing two items: allow and block.
// One AllowBlockMenuButton is used for each requested permission, but only when
// the permission bubble is in 'customize' mode.
@interface AllowBlockMenuButton : NSPopUpButton {
 @private
  scoped_ptr<PermissionMenuModel> menuModel_;
  base::scoped_nsobject<MenuController> menuController_;
}

- (id)initForURL:(const GURL&)url
         allowed:(BOOL)allow
           index:(int)index
        delegate:(PermissionBubbleView::Delegate*)delegate;

// Returns the maximum width of its possible titles.
- (CGFloat)maximumTitleWidth;
@end

@implementation AllowBlockMenuButton

- (id)initForURL:(const GURL&)url
         allowed:(BOOL)allow
           index:(int)index
        delegate:(PermissionBubbleView::Delegate*)delegate {
  if (self = [super initWithFrame:NSZeroRect pullsDown:NO]) {
    ContentSetting setting =
        allow ? CONTENT_SETTING_ALLOW : CONTENT_SETTING_BLOCK;
    [self setFont:[NSFont systemFontOfSize:[NSFont smallSystemFontSize]]];
    [self setBordered:NO];

    __block PermissionBubbleView::Delegate* blockDelegate = delegate;
    __block AllowBlockMenuButton* blockSelf = self;
    PermissionMenuModel::ChangeCallback changeCallback =
        base::BindBlock(^(const WebsiteSettingsUI::PermissionInfo& permission) {
            blockDelegate->ToggleAccept(
                index, permission.setting == CONTENT_SETTING_ALLOW);
            [blockSelf setFrameSize:
                SizeForWebsiteSettingsButtonTitle(blockSelf,
                                                  [blockSelf title])];
        });

    menuModel_.reset(new PermissionMenuModel(url, setting, changeCallback));
    menuController_.reset([[MenuController alloc] initWithModel:menuModel_.get()
                                         useWithPopUpButtonCell:NO]);
    [self setMenu:[menuController_ menu]];
    [self selectItemAtIndex:menuModel_->GetIndexOfCommandId(setting)];
    // Although the frame is reset, below, this sizes the cell properly.
    [self sizeToFit];
    // Adjust the size to fit the current title.  Using only -sizeToFit leaves
    // an ugly amount of whitespace between the title and the arrows because it
    // will fit to the largest element in the menu, not just the selected item.
    [self setFrameSize:SizeForWebsiteSettingsButtonTitle(self, [self title])];
  }
  return self;
}

- (CGFloat)maximumTitleWidth {
  CGFloat maxTitleWidth = 0;
  for (NSMenuItem* item in [self itemArray]) {
    NSSize size = SizeForWebsiteSettingsButtonTitle(self, [item title]);
    maxTitleWidth = std::max(maxTitleWidth, size.width);
  }
  return maxTitleWidth;
}

@end

// The window used for the permission bubble controller.
// Subclassed to allow browser-handled keyboard events to be passed from the
// permission bubble to its parent window, which is a browser window.
@interface PermissionBubbleWindow : InfoBubbleWindow
@end

@implementation PermissionBubbleWindow
- (BOOL)performKeyEquivalent:(NSEvent*)event {
  // Only handle events if they should be forwarded to the parent window.
  if ([self allowShareParentKeyState]) {
    content::NativeWebKeyboardEvent wrappedEvent(event);
    if ([BrowserWindowUtils shouldHandleKeyboardEvent:wrappedEvent]) {
      // Turn off sharing of key window state while the keyboard event is
      // processed.  This avoids recursion - with the key window state shared,
      // the parent window would just forward the event back to this class.
      [self setAllowShareParentKeyState:NO];
      BOOL eventHandled =
          [BrowserWindowUtils handleKeyboardEvent:event
                                         inWindow:[self parentWindow]];
      [self setAllowShareParentKeyState:YES];
      return eventHandled;
    }
  }
  return NO;
}
@end

@interface PermissionBubbleController ()

// Returns an autoreleased NSView displaying the icon and label for |request|.
- (NSView*)labelForRequest:(PermissionBubbleRequest*)request;

// Returns an autoreleased NSView displaying the title for the bubble
// requesting settings for |host|.
- (NSView*)titleWithHostname:(const std::string&)host;

// Returns an autoreleased NSView displaying a menu for |request|.  The
// menu will be initialized as 'allow' if |allow| is YES.
- (NSView*)menuForRequest:(PermissionBubbleRequest*)request
                  atIndex:(int)index
                    allow:(BOOL)allow;

// Returns an autoreleased NSView of a button with |title| and |action|.
- (NSView*)buttonWithTitle:(NSString*)title
                    action:(SEL)action;

// Returns an autoreleased NSView displaying a block button.
- (NSView*)blockButton;

// Returns an autoreleased NSView with a block button and a drop-down menu
// with one item, which will change the UI to allow customizing the permissions.
- (NSView*)blockButtonWithCustomizeMenu;

// Returns an autoreleased NSView displaying the close 'x' button.
- (NSView*)closeButton;

// Called when the 'ok' button is pressed.
- (void)ok:(id)sender;

// Called when the 'allow' button is pressed.
- (void)onAllow:(id)sender;

// Called when the 'block' button is pressed.
- (void)onBlock:(id)sender;

// Called when the 'close' button is pressed.
- (void)onClose:(id)sender;

// Called when the 'customize' button is pressed.
- (void)onCustomize:(id)sender;

// Sets the width of both |viewA| and |viewB| to be the larger of the
// two views' widths.  Does not change either view's origin or height.
+ (CGFloat)matchWidthsOf:(NSView*)viewA andOf:(NSView*)viewB;

// Sets the offset of |viewA| so that its vertical center is aligned with the
// vertical center of |viewB|.
+ (void)alignCenterOf:(NSView*)viewA verticallyToCenterOf:(NSView*)viewB;

@end

@implementation PermissionBubbleController

- (id)initWithParentWindow:(NSWindow*)parentWindow
                    bridge:(PermissionBubbleCocoa*)bridge {
  DCHECK(parentWindow);
  DCHECK(bridge);
  base::scoped_nsobject<PermissionBubbleWindow> window(
      [[PermissionBubbleWindow alloc]
          initWithContentRect:ui::kWindowSizeDeterminedLater
                    styleMask:NSBorderlessWindowMask
                      backing:NSBackingStoreBuffered
                        defer:NO]);
  [window setAllowedAnimations:info_bubble::kAnimateNone];
  [window setReleasedWhenClosed:NO];
  if ((self = [super initWithWindow:window
                       parentWindow:parentWindow
                         anchoredAt:NSZeroPoint])) {
    [self setShouldCloseOnResignKey:NO];
    [self setShouldOpenAsKeyWindow:NO];
    [[self bubble] setArrowLocation:info_bubble::kTopLeft];
    bridge_ = bridge;
    NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
    [center addObserver:self
               selector:@selector(parentWindowDidMove:)
                   name:NSWindowDidMoveNotification
                 object:parentWindow];
  }
  return self;
}

- (void)windowWillClose:(NSNotification*)notification {
  bridge_->OnBubbleClosing();
  [super windowWillClose:notification];
}

- (void)parentWindowWillBecomeFullScreen:(NSNotification*)notification {
  // Override the base class implementation, which would have closed the bubble.
}

- (void)parentWindowDidResize:(NSNotification*)notification {
  DCHECK(bridge_);
  [self setAnchorPoint:bridge_->GetAnchorPoint()];
}

- (void)parentWindowDidMove:(NSNotification*)notification {
  DCHECK(bridge_);
  [self setAnchorPoint:bridge_->GetAnchorPoint()];
}

- (void)showAtAnchor:(NSPoint)anchorPoint
         withDelegate:(PermissionBubbleView::Delegate*)delegate
          forRequests:(const std::vector<PermissionBubbleRequest*>&)requests
         acceptStates:(const std::vector<bool>&)acceptStates
    customizationMode:(BOOL)customizationMode {
  DCHECK(!requests.empty());
  DCHECK(delegate);
  DCHECK(!customizationMode || (requests.size() == acceptStates.size()));
  delegate_ = delegate;

  NSView* contentView = [[self window] contentView];
  [contentView setSubviews:@[]];

  // Create one button to use as a guide for the permissions' y-offsets.
  base::scoped_nsobject<NSView> allowOrOkButton;
  if (customizationMode) {
    NSString* okTitle = l10n_util::GetNSString(IDS_OK);
    allowOrOkButton.reset([[self buttonWithTitle:okTitle
                                          action:@selector(ok:)] retain]);
  } else {
    NSString* allowTitle = l10n_util::GetNSString(IDS_PERMISSION_ALLOW);
    allowOrOkButton.reset([[self buttonWithTitle:allowTitle
                                          action:@selector(onAllow:)] retain]);
  }
  CGFloat yOffset = 2 * kVerticalPadding + NSMaxY([allowOrOkButton frame]);
  BOOL singlePermission = requests.size() == 1;

  base::scoped_nsobject<NSMutableArray> permissionMenus;
  if (customizationMode)
    permissionMenus.reset([[NSMutableArray alloc] init]);

  CGFloat maxPermissionLineWidth = 0;
  for (auto it = requests.begin(); it != requests.end(); it++) {
    base::scoped_nsobject<NSView> permissionView(
        [[self labelForRequest:(*it)] retain]);
    NSPoint origin = [permissionView frame].origin;
    origin.x += kHorizontalPadding;
    origin.y += yOffset;
    [permissionView setFrameOrigin:origin];
    [contentView addSubview:permissionView];

    if (customizationMode) {
      int index = it - requests.begin();
      base::scoped_nsobject<NSView> menu(
          [[self menuForRequest:(*it)
                        atIndex:index
                          allow:acceptStates[index] ? YES : NO] retain]);
      // Align vertically.  Horizontal alignment will be adjusted once the
      // widest permission is know.
      [PermissionBubbleController alignCenterOf:menu
                           verticallyToCenterOf:permissionView];
      [permissionMenus addObject:menu];
      [contentView addSubview:menu];
    }
    maxPermissionLineWidth = std::max(
        maxPermissionLineWidth, NSMaxX([permissionView frame]));
    yOffset += NSHeight([permissionView frame]);
  }

  base::scoped_nsobject<NSView> titleView(
      [[self titleWithHostname:requests[0]->GetRequestingHostname().host()]
          retain]);
  [contentView addSubview:titleView];
  [titleView setFrameOrigin:NSMakePoint(kHorizontalPadding,
                                        kVerticalPadding + yOffset)];

  // 'x' button in the upper-right-hand corner.
  base::scoped_nsobject<NSView> closeButton([[self closeButton] retain]);

  // Determine the dimensions of the bubble.
  // Once the height and width are set, the buttons and permission menus can
  // be laid out correctly.
  NSRect bubbleFrame = NSMakeRect(0, 0, kBubbleMinWidth, 0);

  // Fix the height of the bubble relative to the title.
  bubbleFrame.size.height = NSMaxY([titleView frame]) + kVerticalPadding +
                            info_bubble::kBubbleArrowHeight;

  if (customizationMode) {
    // Add the maximum menu width to the bubble width.
    CGFloat maxMenuWidth = 0;
    for (AllowBlockMenuButton* button in permissionMenus.get()) {
      maxMenuWidth = std::max(maxMenuWidth, [button maximumTitleWidth]);
    }
    maxPermissionLineWidth += maxMenuWidth;
  }

  // The title and 'x' button row must fit within the bubble.
  CGFloat titleRowWidth = NSMaxX([titleView frame]) +
                          NSWidth([closeButton frame]) +
                          chrome_style::kCloseButtonPadding;

  bubbleFrame.size.width = std::max(
      NSWidth(bubbleFrame), std::max(titleRowWidth, maxPermissionLineWidth));

  // Now that the bubble's dimensions have been set, lay out the buttons and
  // menus.

  // Place the close button at the upper-right-hand corner of the bubble.
  NSPoint closeButtonOrigin =
      NSMakePoint(NSWidth(bubbleFrame) - NSWidth([closeButton frame]) -
                      chrome_style::kCloseButtonPadding,
                  NSHeight(bubbleFrame) - NSWidth([closeButton frame]) -
                      chrome_style::kCloseButtonPadding);
  // Account for the bubble's arrow.
  closeButtonOrigin.y -= info_bubble::kBubbleArrowHeight;

  [closeButton setFrameOrigin:closeButtonOrigin];
  [contentView addSubview:closeButton];

  // Position the allow/ok button.
  CGFloat xOrigin = NSWidth(bubbleFrame) - NSWidth([allowOrOkButton frame]) -
                    kButtonRightEdgePadding;
  [allowOrOkButton setFrameOrigin:NSMakePoint(xOrigin, kVerticalPadding)];
  [contentView addSubview:allowOrOkButton];

  if (customizationMode) {
    // Adjust the horizontal origin for each menu so that its right edge
    // lines up with the right edge of the ok button.
    CGFloat rightEdge = NSMaxX([allowOrOkButton frame]);
    for (NSView* view in permissionMenus.get()) {
      [view setFrameOrigin:NSMakePoint(rightEdge - NSWidth([view frame]),
                                       NSMinY([view frame]))];
    }
  } else {
    base::scoped_nsobject<NSView> blockButton;
    if (singlePermission)
      blockButton.reset([[self blockButton] retain]);
    else
      blockButton.reset([[self blockButtonWithCustomizeMenu] retain]);
    CGFloat width = [PermissionBubbleController matchWidthsOf:blockButton
                                                        andOf:allowOrOkButton];
    // Ensure the allow/ok button is still in the correct position.
    xOrigin = NSWidth(bubbleFrame) - width - kHorizontalPadding;
    [allowOrOkButton setFrameOrigin:NSMakePoint(xOrigin, kVerticalPadding)];
    // Line up the block button.
    xOrigin = NSMinX([allowOrOkButton frame]) - width - kBetweenButtonsPadding;
    [blockButton setFrameOrigin:NSMakePoint(xOrigin, kVerticalPadding)];
    [contentView addSubview:blockButton];
  }

  bubbleFrame = [[self window] frameRectForContentRect:bubbleFrame];
  if ([[self window] isVisible]) {
    // Unfortunately, calling -setFrame followed by -setFrameOrigin  (called
    // within -setAnchorPoint) causes flickering.  Avoid the flickering by
    // manually adjusting the new frame's origin so that the top left stays the
    // same, and only calling -setFrame.
    NSRect currentWindowFrame = [[self window] frame];
    bubbleFrame.origin = currentWindowFrame.origin;
    bubbleFrame.origin.y = bubbleFrame.origin.y +
        currentWindowFrame.size.height - bubbleFrame.size.height;
    [[self window] setFrame:bubbleFrame display:YES];
  } else {
    [[self window] setFrame:bubbleFrame display:NO];
    [self setAnchorPoint:anchorPoint];
    [self showWindow:nil];
  }
}

- (NSView*)labelForRequest:(PermissionBubbleRequest*)request {
  DCHECK(request);
  base::scoped_nsobject<NSView> permissionView(
      [[NSView alloc] initWithFrame:NSZeroRect]);
  base::scoped_nsobject<NSImageView> permissionIcon(
      [[NSImageView alloc] initWithFrame:NSZeroRect]);
  [permissionIcon setImage:ui::ResourceBundle::GetSharedInstance().
      GetNativeImageNamed(request->GetIconID()).ToNSImage()];
  [permissionIcon setFrameSize:kPermissionIconSize];
  [permissionView addSubview:permissionIcon];

  base::scoped_nsobject<NSTextField> permissionLabel(
      [[NSTextField alloc] initWithFrame:NSZeroRect]);
  base::string16 label = request->GetMessageTextFragment();
  [permissionLabel setDrawsBackground:NO];
  [permissionLabel setBezeled:NO];
  [permissionLabel setEditable:NO];
  [permissionLabel setSelectable:NO];
  [permissionLabel
      setFont:[NSFont systemFontOfSize:[NSFont smallSystemFontSize]]];
  [permissionLabel setStringValue:base::SysUTF16ToNSString(label)];
  [permissionLabel sizeToFit];
  [permissionLabel setFrameOrigin:
      NSMakePoint(NSWidth([permissionIcon frame]), 0)];
  [permissionView addSubview:permissionLabel];

  // Match the horizontal centers of the two subviews.  Note that the label's
  // center is rounded down, and the icon's center, up.  It looks better that
  // way - with the text's center slightly lower than the icon's center - if the
  // height delta is not evenly split.
  NSRect iconFrame = [permissionIcon frame];
  NSRect labelFrame = [permissionLabel frame];
  NSRect unionFrame = NSUnionRect(iconFrame, labelFrame);

  iconFrame.origin.y =
      std::ceil((NSHeight(unionFrame) - NSHeight(iconFrame)) / 2);
  labelFrame.origin.y =
      std::floor((NSHeight(unionFrame) - NSHeight(labelFrame)) / 2);

  [permissionLabel setFrame:labelFrame];
  [permissionIcon setFrame:iconFrame];
  [permissionView setFrame:unionFrame];

  return permissionView.autorelease();
}

- (NSView*)titleWithHostname:(const std::string&)host {
  base::scoped_nsobject<NSTextField> titleView(
      [[NSTextField alloc] initWithFrame:NSZeroRect]);
  [titleView setDrawsBackground:NO];
  [titleView setBezeled:NO];
  [titleView setEditable:NO];
  [titleView setSelectable:NO];
  [titleView setStringValue:
      l10n_util::GetNSStringF(IDS_PERMISSIONS_BUBBLE_PROMPT,
                              base::UTF8ToUTF16(host))];
  [titleView setFont:[NSFont systemFontOfSize:[NSFont smallSystemFontSize]]];
  [titleView sizeToFit];
  NSRect titleFrame = [titleView frame];
  [titleView setFrameSize:NSMakeSize(NSWidth(titleFrame) + kTitlePaddingX,
                                     NSHeight(titleFrame))];
  return titleView.autorelease();
}

- (NSView*)menuForRequest:(PermissionBubbleRequest*)request
                  atIndex:(int)index
                    allow:(BOOL)allow {
  DCHECK(request);
  DCHECK(delegate_);
  base::scoped_nsobject<AllowBlockMenuButton> button(
      [[AllowBlockMenuButton alloc] initForURL:request->GetRequestingHostname()
                                       allowed:allow
                                         index:index
                                      delegate:delegate_]);
  return button.autorelease();
}

- (NSView*)buttonWithTitle:(NSString*)title
                    action:(SEL)action {
  base::scoped_nsobject<NSButton> button(
      [[ConstrainedWindowButton alloc] initWithFrame:NSZeroRect]);
  [button setButtonType:NSMomentaryPushInButton];
  [button setTitle:title];
  [button setTarget:self];
  [button setAction:action];
  [button sizeToFit];
  return button.autorelease();
}

- (NSView*)blockButton {
  NSString* blockTitle = l10n_util::GetNSString(IDS_PERMISSION_DENY);
  return [self buttonWithTitle:blockTitle
                        action:@selector(onBlock:)];
}

- (NSView*)blockButtonWithCustomizeMenu {
  menuDelegate_.reset(new MenuDelegate(self));
  base::scoped_nsobject<SplitBlockButton> blockButton([[SplitBlockButton alloc]
      initWithMenuDelegate:menuDelegate_.get()]);
  [blockButton sizeToFit];
  [blockButton setEnabled:YES];
  [blockButton setAction:@selector(onBlock:)];
  [blockButton setTarget:self];
  return blockButton.autorelease();
}

- (NSView*)closeButton {
  int dimension = chrome_style::GetCloseButtonSize();
  NSRect frame = NSMakeRect(0, 0, dimension, dimension);
  base::scoped_nsobject<NSButton> button(
      [[WebUIHoverCloseButton alloc] initWithFrame:frame]);
  [button setAction:@selector(onClose:)];
  [button setTarget:self];
  return button.autorelease();
}

- (void)ok:(id)sender {
  DCHECK(delegate_);
  delegate_->Closing();
}

- (void)onAllow:(id)sender {
  DCHECK(delegate_);
  delegate_->Accept();
}

- (void)onBlock:(id)sender {
  DCHECK(delegate_);
  delegate_->Deny();
}

- (void)onClose:(id)sender {
  DCHECK(delegate_);
  delegate_->Closing();
}

- (void)onCustomize:(id)sender {
  DCHECK(delegate_);
  delegate_->SetCustomizationMode();
}

- (void)onMenuItemClicked:(int)commandId {
  DCHECK(commandId == 0);
  [self onCustomize:nil];
}

- (void)activateTabWithContents:(content::WebContents*)newContents
               previousContents:(content::WebContents*)oldContents
                        atIndex:(NSInteger)index
                         reason:(int)reason {
  // The show/hide of this bubble is handled by the PermissionBubbleManager.
  // So bypass the base class, which would close the bubble here.
}

+ (CGFloat)matchWidthsOf:(NSView*)viewA andOf:(NSView*)viewB {
  NSRect frameA = [viewA frame];
  NSRect frameB = [viewB frame];
  CGFloat width = std::max(NSWidth(frameA), NSWidth(frameB));
  [viewA setFrameSize:NSMakeSize(width, NSHeight(frameA))];
  [viewB setFrameSize:NSMakeSize(width, NSHeight(frameB))];
  return width;
}

+ (void)alignCenterOf:(NSView*)viewA verticallyToCenterOf:(NSView*)viewB {
  NSRect frameA = [viewA frame];
  NSRect frameB = [viewB frame];
  frameA.origin.y =
      NSMinY(frameB) + std::floor((NSHeight(frameB) - NSHeight(frameA)) / 2);
  [viewA setFrameOrigin:frameA.origin];
}

@end  // implementation PermissionBubbleController
