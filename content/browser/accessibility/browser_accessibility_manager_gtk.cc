// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/accessibility/browser_accessibility_manager_gtk.h"

#include "content/browser/accessibility/browser_accessibility_gtk.h"
#include "content/common/accessibility_messages.h"

namespace content {

// static
BrowserAccessibilityManager* BrowserAccessibilityManager::Create(
     const AccessibilityNodeData& src,
    BrowserAccessibilityDelegate* delegate,
    BrowserAccessibilityFactory* factory) {
  return new BrowserAccessibilityManagerGtk(
      NULL,
      src,
      delegate,
      factory);
}

BrowserAccessibilityManagerGtk::BrowserAccessibilityManagerGtk(
    GtkWidget* parent_widget,
    const AccessibilityNodeData& src,
    BrowserAccessibilityDelegate* delegate,
    BrowserAccessibilityFactory* factory)
    : BrowserAccessibilityManager(delegate, factory),
      parent_widget_(parent_widget) {
  Initialize(src);
}

BrowserAccessibilityManagerGtk::~BrowserAccessibilityManagerGtk() {
}

// static
AccessibilityNodeData BrowserAccessibilityManagerGtk::GetEmptyDocument() {
  AccessibilityNodeData empty_document;
  empty_document.id = 0;
  empty_document.role = blink::WebAXRoleRootWebArea;
  empty_document.state =
      1 << blink::WebAXStateReadonly;
  return empty_document;
}

void BrowserAccessibilityManagerGtk::NotifyAccessibilityEvent(
    blink::WebAXEvent event_type,
    BrowserAccessibility* node) {
  if (!node->IsNative())
    return;
  AtkObject* atk_object = node->ToBrowserAccessibilityGtk()->GetAtkObject();

  switch (event_type) {
    case blink::WebAXEventChildrenChanged:
      RecursivelySendChildrenChanged(GetRoot()->ToBrowserAccessibilityGtk());
      break;
    case blink::WebAXEventFocus:
      // Note: the focus-event was deprecated in ATK 2.9.4
      // See https://bugzilla.gnome.org/show_bug.cgi?id=649575#c8
      g_signal_emit_by_name(atk_object, "focus-event", true);
      break;
    default:
      break;
  }
}

void BrowserAccessibilityManagerGtk::RecursivelySendChildrenChanged(
    BrowserAccessibilityGtk* node) {
  AtkObject* atkObject = node->ToBrowserAccessibilityGtk()->GetAtkObject();
  for (unsigned int i = 0; i < node->children().size(); ++i) {
    BrowserAccessibilityGtk* child =
        node->children()[i]->ToBrowserAccessibilityGtk();
    g_signal_emit_by_name(atkObject,
                          "children-changed::add",
                          i,
                          child->GetAtkObject());
    RecursivelySendChildrenChanged(child);
  }
}

}  // namespace content
