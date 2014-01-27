// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/mac/scoped_nsautorelease_pool.h"
#include "base/mac/scoped_nsobject.h"
#include "base/strings/sys_string_conversions.h"
#include "remoting/base/string_resources.h"
#include "remoting/host/continue_window.h"
#include "ui/base/l10n/l10n_util_mac.h"

// Handles the ContinueWindow.
@interface ContinueWindowMacController : NSObject {
 @private
  base::scoped_nsobject<NSMutableArray> shades_;
  base::scoped_nsobject<NSAlert> continue_alert_;
  remoting::ContinueWindow* continue_window_;
}

- (id)initWithWindow:(remoting::ContinueWindow*)continue_window;
- (void)show;
- (void)hide;
- (void)onCancel:(id)sender;
- (void)onContinue:(id)sender;
@end

namespace remoting {

// A bridge between C++ and ObjC implementations of ContinueWindow.
// Everything important occurs in ContinueWindowMacController.
class ContinueWindowMac : public ContinueWindow {
 public:
  ContinueWindowMac();
  virtual ~ContinueWindowMac();

 protected:
  // ContinueWindow overrides.
  virtual void ShowUi() OVERRIDE;
  virtual void HideUi() OVERRIDE;

 private:
  base::scoped_nsobject<ContinueWindowMacController> controller_;

  DISALLOW_COPY_AND_ASSIGN(ContinueWindowMac);
};

ContinueWindowMac::ContinueWindowMac() {
}

ContinueWindowMac::~ContinueWindowMac() {
  DCHECK(CalledOnValidThread());
}

void ContinueWindowMac::ShowUi() {
  DCHECK(CalledOnValidThread());

  base::mac::ScopedNSAutoreleasePool pool;
  controller_.reset(
      [[ContinueWindowMacController alloc] initWithWindow:this]);
  [controller_ show];
}

void ContinueWindowMac::HideUi() {
  DCHECK(CalledOnValidThread());

  base::mac::ScopedNSAutoreleasePool pool;
  [controller_ hide];
}

// static
scoped_ptr<HostWindow> HostWindow::CreateContinueWindow() {
  return scoped_ptr<HostWindow>(new ContinueWindowMac());
}

}  // namespace remoting

@implementation ContinueWindowMacController

- (id)initWithWindow:(remoting::ContinueWindow*)continue_window {
  if ((self = [super init])) {
    continue_window_ = continue_window;
  }
  return self;
}

- (void)show {
  // Generate window shade
  NSArray* screens = [NSScreen screens];
  shades_.reset([[NSMutableArray alloc] initWithCapacity:[screens count]]);
  for (NSScreen *screen in screens) {
    NSWindow* shade =
      [[[NSWindow alloc] initWithContentRect:[screen frame]
                                   styleMask:NSBorderlessWindowMask
                                     backing:NSBackingStoreBuffered
                                       defer:NO
                                      screen:screen] autorelease];
    [shade setReleasedWhenClosed:NO];
    [shade setAlphaValue:0.8];
    [shade setOpaque:NO];
    [shade setBackgroundColor:[NSColor blackColor]];
    // Raise the window shade above just about everything else.
    // Leave the dock and menu bar exposed so the user has some basic level
    // of control (like they can quit Chromium).
    [shade setLevel:NSModalPanelWindowLevel - 1];
    [shade orderFront:nil];
    [shades_ addObject:shade];
  }

  // Create alert.
  continue_alert_.reset([[NSAlert alloc] init]);
  [continue_alert_ setMessageText:l10n_util::GetNSString(IDS_CONTINUE_PROMPT)];

  NSButton* continue_button =
      [continue_alert_ addButtonWithTitle:l10n_util::GetNSString(
          IDS_CONTINUE_BUTTON)];
  [continue_button setAction:@selector(onContinue:)];
  [continue_button setTarget:self];

  NSButton* cancel_button =
      [continue_alert_ addButtonWithTitle:l10n_util::GetNSString(
          IDS_STOP_SHARING_BUTTON)];
  [cancel_button setAction:@selector(onCancel:)];
  [cancel_button setTarget:self];

  NSBundle *bundle = [NSBundle bundleForClass:[self class]];
  NSString *imagePath = [bundle pathForResource:@"chromoting128" ofType:@"png"];
  base::scoped_nsobject<NSImage> image(
      [[NSImage alloc] initByReferencingFile:imagePath]);
  [continue_alert_ setIcon:image];
  [continue_alert_ layout];

  // Force alert to be at the proper level and location.
  NSWindow* continue_window = [continue_alert_ window];
  [continue_window center];
  [continue_window setLevel:NSModalPanelWindowLevel];
  [continue_window orderWindow:NSWindowAbove
                    relativeTo:[[shades_ lastObject] windowNumber]];
  [continue_window makeKeyWindow];
}

- (void)hide {
  // Remove window shade.
  for (NSWindow* window in shades_.get()) {
    [window close];
  }
  shades_.reset();
  continue_alert_.reset();
}

- (void)onCancel:(id)sender {
  [self hide];
  continue_window_->DisconnectSession();
}

- (void)onContinue:(id)sender {
  [self hide];
  continue_window_->ContinueSession();
}

@end
