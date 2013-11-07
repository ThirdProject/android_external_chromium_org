// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "content/browser/accessibility/browser_accessibility.h"
#include "content/browser/accessibility/browser_accessibility_manager.h"
#include "content/common/accessibility_messages.h"
#include "content/common/accessibility_node_data.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {
namespace {

// Subclass of BrowserAccessibility that counts the number of instances.
class CountedBrowserAccessibility : public BrowserAccessibility {
 public:
  CountedBrowserAccessibility() {
    global_obj_count_++;
    native_ref_count_ = 1;
  }
  virtual ~CountedBrowserAccessibility() {
    global_obj_count_--;
  }

  virtual void NativeAddReference() OVERRIDE {
    native_ref_count_++;
  }

  virtual void NativeReleaseReference() OVERRIDE {
    native_ref_count_--;
    if (native_ref_count_ == 0)
      delete this;
  }

  int native_ref_count_;
  static int global_obj_count_;
};

int CountedBrowserAccessibility::global_obj_count_ = 0;

// Factory that creates a CountedBrowserAccessibility.
class CountedBrowserAccessibilityFactory
    : public BrowserAccessibilityFactory {
 public:
  virtual ~CountedBrowserAccessibilityFactory() {}
  virtual BrowserAccessibility* Create() OVERRIDE {
    return new CountedBrowserAccessibility();
  }
};

class TestBrowserAccessibilityDelegate
    : public BrowserAccessibilityDelegate {
 public:
  TestBrowserAccessibilityDelegate()
      : got_fatal_error_(false) {}

  virtual void SetAccessibilityFocus(int acc_obj_id) OVERRIDE {}
  virtual void AccessibilityDoDefaultAction(int acc_obj_id) OVERRIDE {}
  virtual void AccessibilityScrollToMakeVisible(
      int acc_obj_id, gfx::Rect subfocus) OVERRIDE {}
  virtual void AccessibilityScrollToPoint(
      int acc_obj_id, gfx::Point point) OVERRIDE {}
  virtual void AccessibilitySetTextSelection(
      int acc_obj_id, int start_offset, int end_offset) OVERRIDE {}
  virtual bool HasFocus() const OVERRIDE {
    return false;
  }
  virtual gfx::Rect GetViewBounds() const OVERRIDE {
    return gfx::Rect();
  }
  virtual gfx::Point GetLastTouchEventLocation() const OVERRIDE {
    return gfx::Point();
  }
  virtual void FatalAccessibilityTreeError() OVERRIDE {
    got_fatal_error_ = true;
  }

  bool got_fatal_error() const { return got_fatal_error_; }
  void reset_got_fatal_error() { got_fatal_error_ = false; }

private:
  bool got_fatal_error_;
};

}  // anonymous namespace

TEST(BrowserAccessibilityManagerTest, TestNoLeaks) {
  // Create AccessibilityNodeData objects for a simple document tree,
  // representing the accessibility information used to initialize
  // BrowserAccessibilityManager.
  AccessibilityNodeData button;
  button.id = 2;
  button.SetName("Button");
  button.role = blink::WebAXRoleButton;
  button.state = 0;

  AccessibilityNodeData checkbox;
  checkbox.id = 3;
  checkbox.SetName("Checkbox");
  checkbox.role = blink::WebAXRoleCheckBox;
  checkbox.state = 0;

  AccessibilityNodeData root;
  root.id = 1;
  root.SetName("Document");
  root.role = blink::WebAXRoleRootWebArea;
  root.state = 0;
  root.child_ids.push_back(2);
  root.child_ids.push_back(3);

  // Construct a BrowserAccessibilityManager with this
  // AccessibilityNodeData tree and a factory for an instance-counting
  // BrowserAccessibility, and ensure that exactly 3 instances were
  // created. Note that the manager takes ownership of the factory.
  CountedBrowserAccessibility::global_obj_count_ = 0;
  BrowserAccessibilityManager* manager =
      BrowserAccessibilityManager::Create(
          root,
          NULL,
          new CountedBrowserAccessibilityFactory());
  manager->UpdateNodesForTesting(button, checkbox);

  ASSERT_EQ(3, CountedBrowserAccessibility::global_obj_count_);

  // Delete the manager and test that all 3 instances are deleted.
  delete manager;
  ASSERT_EQ(0, CountedBrowserAccessibility::global_obj_count_);

  // Construct a manager again, and this time save references to two of
  // the three nodes in the tree.
  manager =
      BrowserAccessibilityManager::Create(
          root,
          NULL,
          new CountedBrowserAccessibilityFactory());
  manager->UpdateNodesForTesting(button, checkbox);
  ASSERT_EQ(3, CountedBrowserAccessibility::global_obj_count_);

  CountedBrowserAccessibility* root_accessible =
      static_cast<CountedBrowserAccessibility*>(manager->GetRoot());
  root_accessible->NativeAddReference();
  CountedBrowserAccessibility* child1_accessible =
      static_cast<CountedBrowserAccessibility*>(
          root_accessible->PlatformGetChild(1));
  child1_accessible->NativeAddReference();

  // Now delete the manager, and only one of the three nodes in the tree
  // should be released.
  delete manager;
  ASSERT_EQ(2, CountedBrowserAccessibility::global_obj_count_);

  // Release each of our references and make sure that each one results in
  // the instance being deleted as its reference count hits zero.
  root_accessible->NativeReleaseReference();
  ASSERT_EQ(1, CountedBrowserAccessibility::global_obj_count_);
  child1_accessible->NativeReleaseReference();
  ASSERT_EQ(0, CountedBrowserAccessibility::global_obj_count_);
}

TEST(BrowserAccessibilityManagerTest, TestReuseBrowserAccessibilityObjects) {
  // Make sure that changes to a subtree reuse as many objects as possible.

  // Tree 1:
  //
  // root
  //   child1
  //   child2
  //   child3

  AccessibilityNodeData tree1_child1;
  tree1_child1.id = 2;
  tree1_child1.SetName("Child1");
  tree1_child1.role = blink::WebAXRoleButton;
  tree1_child1.state = 0;

  AccessibilityNodeData tree1_child2;
  tree1_child2.id = 3;
  tree1_child2.SetName("Child2");
  tree1_child2.role = blink::WebAXRoleButton;
  tree1_child2.state = 0;

  AccessibilityNodeData tree1_child3;
  tree1_child3.id = 4;
  tree1_child3.SetName("Child3");
  tree1_child3.role = blink::WebAXRoleButton;
  tree1_child3.state = 0;

  AccessibilityNodeData tree1_root;
  tree1_root.id = 1;
  tree1_root.SetName("Document");
  tree1_root.role = blink::WebAXRoleRootWebArea;
  tree1_root.state = 0;
  tree1_root.child_ids.push_back(2);
  tree1_root.child_ids.push_back(3);
  tree1_root.child_ids.push_back(4);

  // Tree 2:
  //
  // root
  //   child0  <-- inserted
  //   child1
  //   child2
  //           <-- child3 deleted

  AccessibilityNodeData tree2_child0;
  tree2_child0.id = 5;
  tree2_child0.SetName("Child0");
  tree2_child0.role = blink::WebAXRoleButton;
  tree2_child0.state = 0;

  AccessibilityNodeData tree2_root;
  tree2_root.id = 1;
  tree2_root.SetName("DocumentChanged");
  tree2_root.role = blink::WebAXRoleRootWebArea;
  tree2_root.state = 0;
  tree2_root.child_ids.push_back(5);
  tree2_root.child_ids.push_back(2);
  tree2_root.child_ids.push_back(3);

  // Construct a BrowserAccessibilityManager with tree1.
  CountedBrowserAccessibility::global_obj_count_ = 0;
  BrowserAccessibilityManager* manager =
      BrowserAccessibilityManager::Create(
          tree1_root,
          NULL,
          new CountedBrowserAccessibilityFactory());
  manager->UpdateNodesForTesting(tree1_child1, tree1_child2, tree1_child3);
  ASSERT_EQ(4, CountedBrowserAccessibility::global_obj_count_);

  // Save references to all of the objects.
  CountedBrowserAccessibility* root_accessible =
      static_cast<CountedBrowserAccessibility*>(manager->GetRoot());
  root_accessible->NativeAddReference();
  CountedBrowserAccessibility* child1_accessible =
      static_cast<CountedBrowserAccessibility*>(
          root_accessible->PlatformGetChild(0));
  child1_accessible->NativeAddReference();
  CountedBrowserAccessibility* child2_accessible =
      static_cast<CountedBrowserAccessibility*>(
          root_accessible->PlatformGetChild(1));
  child2_accessible->NativeAddReference();
  CountedBrowserAccessibility* child3_accessible =
      static_cast<CountedBrowserAccessibility*>(
          root_accessible->PlatformGetChild(2));
  child3_accessible->NativeAddReference();

  // Check the index in parent.
  EXPECT_EQ(0, child1_accessible->index_in_parent());
  EXPECT_EQ(1, child2_accessible->index_in_parent());
  EXPECT_EQ(2, child3_accessible->index_in_parent());

  // Process a notification containing the changed subtree.
  std::vector<AccessibilityHostMsg_EventParams> params;
  params.push_back(AccessibilityHostMsg_EventParams());
  AccessibilityHostMsg_EventParams* msg = &params[0];
  msg->event_type = blink::WebAXEventChildrenChanged;
  msg->nodes.push_back(tree2_root);
  msg->nodes.push_back(tree2_child0);
  msg->id = tree2_root.id;
  manager->OnAccessibilityEvents(params);

  // There should be 5 objects now: the 4 from the new tree, plus the
  // reference to child3 we kept.
  EXPECT_EQ(5, CountedBrowserAccessibility::global_obj_count_);

  // Check that our references to the root, child1, and child2 are still valid,
  // but that the reference to child3 is now invalid.
  EXPECT_TRUE(root_accessible->instance_active());
  EXPECT_TRUE(child1_accessible->instance_active());
  EXPECT_TRUE(child2_accessible->instance_active());
  EXPECT_FALSE(child3_accessible->instance_active());

  // Check that the index in parent has been updated.
  EXPECT_EQ(1, child1_accessible->index_in_parent());
  EXPECT_EQ(2, child2_accessible->index_in_parent());

  // Release our references. The object count should only decrease by 1
  // for child3.
  root_accessible->NativeReleaseReference();
  child1_accessible->NativeReleaseReference();
  child2_accessible->NativeReleaseReference();
  child3_accessible->NativeReleaseReference();

  EXPECT_EQ(4, CountedBrowserAccessibility::global_obj_count_);

  // Delete the manager and make sure all memory is cleaned up.
  delete manager;
  ASSERT_EQ(0, CountedBrowserAccessibility::global_obj_count_);
}

TEST(BrowserAccessibilityManagerTest, TestReuseBrowserAccessibilityObjects2) {
  // Similar to the test above, but with a more complicated tree.

  // Tree 1:
  //
  // root
  //   container
  //     child1
  //       grandchild1
  //     child2
  //       grandchild2
  //     child3
  //       grandchild3

  AccessibilityNodeData tree1_grandchild1;
  tree1_grandchild1.id = 4;
  tree1_grandchild1.SetName("GrandChild1");
  tree1_grandchild1.role = blink::WebAXRoleButton;
  tree1_grandchild1.state = 0;

  AccessibilityNodeData tree1_child1;
  tree1_child1.id = 3;
  tree1_child1.SetName("Child1");
  tree1_child1.role = blink::WebAXRoleButton;
  tree1_child1.state = 0;
  tree1_child1.child_ids.push_back(4);

  AccessibilityNodeData tree1_grandchild2;
  tree1_grandchild2.id = 6;
  tree1_grandchild2.SetName("GrandChild1");
  tree1_grandchild2.role = blink::WebAXRoleButton;
  tree1_grandchild2.state = 0;

  AccessibilityNodeData tree1_child2;
  tree1_child2.id = 5;
  tree1_child2.SetName("Child2");
  tree1_child2.role = blink::WebAXRoleButton;
  tree1_child2.state = 0;
  tree1_child2.child_ids.push_back(6);

  AccessibilityNodeData tree1_grandchild3;
  tree1_grandchild3.id = 8;
  tree1_grandchild3.SetName("GrandChild3");
  tree1_grandchild3.role = blink::WebAXRoleButton;
  tree1_grandchild3.state = 0;

  AccessibilityNodeData tree1_child3;
  tree1_child3.id = 7;
  tree1_child3.SetName("Child3");
  tree1_child3.role = blink::WebAXRoleButton;
  tree1_child3.state = 0;
  tree1_child3.child_ids.push_back(8);

  AccessibilityNodeData tree1_container;
  tree1_container.id = 2;
  tree1_container.SetName("Container");
  tree1_container.role = blink::WebAXRoleGroup;
  tree1_container.state = 0;
  tree1_container.child_ids.push_back(3);
  tree1_container.child_ids.push_back(5);
  tree1_container.child_ids.push_back(7);

  AccessibilityNodeData tree1_root;
  tree1_root.id = 1;
  tree1_root.SetName("Document");
  tree1_root.role = blink::WebAXRoleRootWebArea;
  tree1_root.state = 0;
  tree1_root.child_ids.push_back(2);

  // Tree 2:
  //
  // root
  //   container
  //     child0         <-- inserted
  //       grandchild0  <--
  //     child1
  //       grandchild1
  //     child2
  //       grandchild2
  //                    <-- child3 (and grandchild3) deleted

  AccessibilityNodeData tree2_grandchild0;
  tree2_grandchild0.id = 9;
  tree2_grandchild0.SetName("GrandChild0");
  tree2_grandchild0.role = blink::WebAXRoleButton;
  tree2_grandchild0.state = 0;

  AccessibilityNodeData tree2_child0;
  tree2_child0.id = 10;
  tree2_child0.SetName("Child0");
  tree2_child0.role = blink::WebAXRoleButton;
  tree2_child0.state = 0;
  tree2_child0.child_ids.push_back(9);

  AccessibilityNodeData tree2_container;
  tree2_container.id = 2;
  tree2_container.SetName("Container");
  tree2_container.role = blink::WebAXRoleGroup;
  tree2_container.state = 0;
  tree2_container.child_ids.push_back(10);
  tree2_container.child_ids.push_back(3);
  tree2_container.child_ids.push_back(5);

  // Construct a BrowserAccessibilityManager with tree1.
  CountedBrowserAccessibility::global_obj_count_ = 0;
  BrowserAccessibilityManager* manager =
      BrowserAccessibilityManager::Create(
          tree1_root,
          NULL,
          new CountedBrowserAccessibilityFactory());
  manager->UpdateNodesForTesting(tree1_container,
                                 tree1_child1, tree1_grandchild1,
                                 tree1_child2, tree1_grandchild2,
                                 tree1_child3, tree1_grandchild3);
  ASSERT_EQ(8, CountedBrowserAccessibility::global_obj_count_);

  // Save references to some objects.
  CountedBrowserAccessibility* root_accessible =
      static_cast<CountedBrowserAccessibility*>(manager->GetRoot());
  root_accessible->NativeAddReference();
  CountedBrowserAccessibility* container_accessible =
      static_cast<CountedBrowserAccessibility*>(
          root_accessible->PlatformGetChild(0));
  container_accessible->NativeAddReference();
  CountedBrowserAccessibility* child2_accessible =
      static_cast<CountedBrowserAccessibility*>(
          container_accessible->PlatformGetChild(1));
  child2_accessible->NativeAddReference();
  CountedBrowserAccessibility* child3_accessible =
      static_cast<CountedBrowserAccessibility*>(
          container_accessible->PlatformGetChild(2));
  child3_accessible->NativeAddReference();

  // Check the index in parent.
  EXPECT_EQ(1, child2_accessible->index_in_parent());
  EXPECT_EQ(2, child3_accessible->index_in_parent());

  // Process a notification containing the changed subtree rooted at
  // the container.
  std::vector<AccessibilityHostMsg_EventParams> params;
  params.push_back(AccessibilityHostMsg_EventParams());
  AccessibilityHostMsg_EventParams* msg = &params[0];
  msg->event_type = blink::WebAXEventChildrenChanged;
  msg->nodes.push_back(tree2_container);
  msg->nodes.push_back(tree2_child0);
  msg->nodes.push_back(tree2_grandchild0);
  msg->id = tree2_container.id;
  manager->OnAccessibilityEvents(params);

  // There should be 9 objects now: the 8 from the new tree, plus the
  // reference to child3 we kept.
  EXPECT_EQ(9, CountedBrowserAccessibility::global_obj_count_);

  // Check that our references to the root and container and child2 are
  // still valid, but that the reference to child3 is now invalid.
  EXPECT_TRUE(root_accessible->instance_active());
  EXPECT_TRUE(container_accessible->instance_active());
  EXPECT_TRUE(child2_accessible->instance_active());
  EXPECT_FALSE(child3_accessible->instance_active());

  // Ensure that we retain the parent of the detached subtree.
  EXPECT_EQ(root_accessible, container_accessible->parent());
  EXPECT_EQ(0, container_accessible->index_in_parent());

  // Check that the index in parent has been updated.
  EXPECT_EQ(2, child2_accessible->index_in_parent());

  // Release our references. The object count should only decrease by 1
  // for child3.
  root_accessible->NativeReleaseReference();
  container_accessible->NativeReleaseReference();
  child2_accessible->NativeReleaseReference();
  child3_accessible->NativeReleaseReference();

  EXPECT_EQ(8, CountedBrowserAccessibility::global_obj_count_);

  // Delete the manager and make sure all memory is cleaned up.
  delete manager;
  ASSERT_EQ(0, CountedBrowserAccessibility::global_obj_count_);
}

TEST(BrowserAccessibilityManagerTest, TestMoveChildUp) {
  // Tree 1:
  //
  // 1
  //   2
  //   3
  //     4

  AccessibilityNodeData tree1_4;
  tree1_4.id = 4;
  tree1_4.state = 0;

  AccessibilityNodeData tree1_3;
  tree1_3.id = 3;
  tree1_3.state = 0;
  tree1_3.child_ids.push_back(4);

  AccessibilityNodeData tree1_2;
  tree1_2.id = 2;
  tree1_2.state = 0;

  AccessibilityNodeData tree1_1;
  tree1_1.id = 1;
  tree1_1.role = blink::WebAXRoleRootWebArea;
  tree1_1.state = 0;
  tree1_1.child_ids.push_back(2);
  tree1_1.child_ids.push_back(3);

  // Tree 2:
  //
  // 1
  //   4    <-- moves up a level and gains child
  //     6  <-- new
  //   5    <-- new

  AccessibilityNodeData tree2_6;
  tree2_6.id = 6;
  tree2_6.state = 0;

  AccessibilityNodeData tree2_5;
  tree2_5.id = 5;
  tree2_5.state = 0;

  AccessibilityNodeData tree2_4;
  tree2_4.id = 4;
  tree2_4.state = 0;
  tree2_4.child_ids.push_back(6);

  AccessibilityNodeData tree2_1;
  tree2_1.id = 1;
  tree2_1.state = 0;
  tree2_1.child_ids.push_back(4);
  tree2_1.child_ids.push_back(5);

  // Construct a BrowserAccessibilityManager with tree1.
  CountedBrowserAccessibility::global_obj_count_ = 0;
  BrowserAccessibilityManager* manager =
      BrowserAccessibilityManager::Create(
          tree1_1,
          NULL,
          new CountedBrowserAccessibilityFactory());
  manager->UpdateNodesForTesting(tree1_2, tree1_3, tree1_4);
  ASSERT_EQ(4, CountedBrowserAccessibility::global_obj_count_);

  // Process a notification containing the changed subtree.
  std::vector<AccessibilityHostMsg_EventParams> params;
  params.push_back(AccessibilityHostMsg_EventParams());
  AccessibilityHostMsg_EventParams* msg = &params[0];
  msg->event_type = blink::WebAXEventChildrenChanged;
  msg->nodes.push_back(tree2_1);
  msg->nodes.push_back(tree2_4);
  msg->nodes.push_back(tree2_5);
  msg->nodes.push_back(tree2_6);
  msg->id = tree2_1.id;
  manager->OnAccessibilityEvents(params);

  // There should be 4 objects now.
  EXPECT_EQ(4, CountedBrowserAccessibility::global_obj_count_);

  // Delete the manager and make sure all memory is cleaned up.
  delete manager;
  ASSERT_EQ(0, CountedBrowserAccessibility::global_obj_count_);
}

// Crashes on Windows. http://crbug.com/304130
#if defined(OS_WIN)
#define MAYBE_TestFatalError DISABLED_TestFatalError
#else
#define MAYBE_TestFatalError TestFatalError
#endif
TEST(BrowserAccessibilityManagerTest, TestFatalError) {
  // Test that BrowserAccessibilityManager raises a fatal error
  // (which will crash the renderer) if the same id is used in
  // two places in the tree.

  AccessibilityNodeData root;
  root.id = 1;
  root.role = blink::WebAXRoleRootWebArea;
  root.child_ids.push_back(2);
  root.child_ids.push_back(2);

  CountedBrowserAccessibilityFactory* factory =
      new CountedBrowserAccessibilityFactory();
  scoped_ptr<TestBrowserAccessibilityDelegate> delegate(
      new TestBrowserAccessibilityDelegate());
  scoped_ptr<BrowserAccessibilityManager> manager;
  ASSERT_FALSE(delegate->got_fatal_error());
  manager.reset(BrowserAccessibilityManager::Create(
      root,
      delegate.get(),
      factory));
  ASSERT_TRUE(delegate->got_fatal_error());

  AccessibilityNodeData root2;
  root2.id = 1;
  root2.role = blink::WebAXRoleRootWebArea;
  root2.child_ids.push_back(2);
  root2.child_ids.push_back(3);

  AccessibilityNodeData child1;
  child1.id = 2;
  child1.child_ids.push_back(4);
  child1.child_ids.push_back(5);

  AccessibilityNodeData child2;
  child2.id = 3;
  child2.child_ids.push_back(6);
  child2.child_ids.push_back(5);  // Duplicate

  delegate->reset_got_fatal_error();
  factory = new CountedBrowserAccessibilityFactory();
  manager.reset(BrowserAccessibilityManager::Create(
      root2,
      delegate.get(),
      factory));
  ASSERT_FALSE(delegate->got_fatal_error());
  manager->UpdateNodesForTesting(child1, child2);
  ASSERT_TRUE(delegate->got_fatal_error());
}

TEST(BrowserAccessibilityManagerTest, BoundsForRange) {
  AccessibilityNodeData root;
  root.id = 1;
  root.role = blink::WebAXRoleRootWebArea;

  AccessibilityNodeData static_text;
  static_text.id = 2;
  static_text.SetValue("Hello, world.");
  static_text.role = blink::WebAXRoleStaticText;
  static_text.location = gfx::Rect(100, 100, 29, 18);
  root.child_ids.push_back(2);

  AccessibilityNodeData inline_text1;
  inline_text1.id = 3;
  inline_text1.SetValue("Hello, ");
  inline_text1.role = blink::WebAXRoleInlineTextBox;
  inline_text1.location = gfx::Rect(100, 100, 29, 9);
  inline_text1.AddIntAttribute(AccessibilityNodeData::ATTR_TEXT_DIRECTION,
                               blink::WebAXTextDirectionLR);
  std::vector<int32> character_offsets1;
  character_offsets1.push_back(6);   // 0
  character_offsets1.push_back(11);  // 1
  character_offsets1.push_back(16);  // 2
  character_offsets1.push_back(21);  // 3
  character_offsets1.push_back(26);  // 4
  character_offsets1.push_back(29);  // 5
  character_offsets1.push_back(29);  // 6 (note that the space has no width)
  inline_text1.AddIntListAttribute(
      AccessibilityNodeData::ATTR_CHARACTER_OFFSETS, character_offsets1);
  static_text.child_ids.push_back(3);

  AccessibilityNodeData inline_text2;
  inline_text2.id = 4;
  inline_text2.SetValue("world.");
  inline_text2.role = blink::WebAXRoleInlineTextBox;
  inline_text2.location = gfx::Rect(100, 109, 28, 9);
  inline_text2.AddIntAttribute(AccessibilityNodeData::ATTR_TEXT_DIRECTION,
                               blink::WebAXTextDirectionLR);
  std::vector<int32> character_offsets2;
  character_offsets2.push_back(5);
  character_offsets2.push_back(10);
  character_offsets2.push_back(15);
  character_offsets2.push_back(20);
  character_offsets2.push_back(25);
  character_offsets2.push_back(28);
  inline_text2.AddIntListAttribute(
      AccessibilityNodeData::ATTR_CHARACTER_OFFSETS, character_offsets2);
  static_text.child_ids.push_back(4);

  scoped_ptr<BrowserAccessibilityManager> manager(
      BrowserAccessibilityManager::Create(
          root,
          NULL,
          new CountedBrowserAccessibilityFactory()));
  manager->UpdateNodesForTesting(static_text, inline_text1, inline_text2);

  BrowserAccessibility* root_accessible = manager->GetRoot();
  BrowserAccessibility* static_text_accessible =
      root_accessible->PlatformGetChild(0);

  EXPECT_EQ(gfx::Rect(100, 100, 6, 9).ToString(),
            static_text_accessible->GetLocalBoundsForRange(0, 1).ToString());

  EXPECT_EQ(gfx::Rect(100, 100, 26, 9).ToString(),
            static_text_accessible->GetLocalBoundsForRange(0, 5).ToString());

  EXPECT_EQ(gfx::Rect(100, 109, 5, 9).ToString(),
            static_text_accessible->GetLocalBoundsForRange(7, 1).ToString());

  EXPECT_EQ(gfx::Rect(100, 109, 25, 9).ToString(),
            static_text_accessible->GetLocalBoundsForRange(7, 5).ToString());

  EXPECT_EQ(gfx::Rect(100, 100, 29, 18).ToString(),
            static_text_accessible->GetLocalBoundsForRange(5, 3).ToString());

  EXPECT_EQ(gfx::Rect(100, 100, 29, 18).ToString(),
            static_text_accessible->GetLocalBoundsForRange(0, 13).ToString());

  // Test range that's beyond the text.
  EXPECT_EQ(gfx::Rect(100, 100, 29, 18).ToString(),
            static_text_accessible->GetLocalBoundsForRange(-1, 999).ToString());
}

TEST(BrowserAccessibilityManagerTest, BoundsForRangeBiDi) {
  // In this example, we assume that the string "123abc" is rendered with
  // "123" going left-to-right and "abc" going right-to-left. In other
  // words, on-screen it would look like "123cba". This is possible to
  // acheive if the source string had unicode control characters
  // to switch directions. This test doesn't worry about how, though - it just
  // tests that if something like that were to occur, GetLocalBoundsForRange
  // returns the correct bounds for different ranges.

  AccessibilityNodeData root;
  root.id = 1;
  root.role = blink::WebAXRoleRootWebArea;

  AccessibilityNodeData static_text;
  static_text.id = 2;
  static_text.SetValue("123abc");
  static_text.role = blink::WebAXRoleStaticText;
  static_text.location = gfx::Rect(100, 100, 60, 20);
  root.child_ids.push_back(2);

  AccessibilityNodeData inline_text1;
  inline_text1.id = 3;
  inline_text1.SetValue("123");
  inline_text1.role = blink::WebAXRoleInlineTextBox;
  inline_text1.location = gfx::Rect(100, 100, 30, 20);
  inline_text1.AddIntAttribute(AccessibilityNodeData::ATTR_TEXT_DIRECTION,
                               blink::WebAXTextDirectionLR);
  std::vector<int32> character_offsets1;
  character_offsets1.push_back(10);  // 0
  character_offsets1.push_back(20);  // 1
  character_offsets1.push_back(30);  // 2
  inline_text1.AddIntListAttribute(
      AccessibilityNodeData::ATTR_CHARACTER_OFFSETS, character_offsets1);
  static_text.child_ids.push_back(3);

  AccessibilityNodeData inline_text2;
  inline_text2.id = 4;
  inline_text2.SetValue("abc");
  inline_text2.role = blink::WebAXRoleInlineTextBox;
  inline_text2.location = gfx::Rect(130, 100, 30, 20);
  inline_text2.AddIntAttribute(AccessibilityNodeData::ATTR_TEXT_DIRECTION,
                               blink::WebAXTextDirectionRL);
  std::vector<int32> character_offsets2;
  character_offsets2.push_back(10);
  character_offsets2.push_back(20);
  character_offsets2.push_back(30);
  inline_text2.AddIntListAttribute(
      AccessibilityNodeData::ATTR_CHARACTER_OFFSETS, character_offsets2);
  static_text.child_ids.push_back(4);

  scoped_ptr<BrowserAccessibilityManager> manager(
      BrowserAccessibilityManager::Create(
          root,
          NULL,
          new CountedBrowserAccessibilityFactory()));
  manager->UpdateNodesForTesting(static_text, inline_text1, inline_text2);

  BrowserAccessibility* root_accessible = manager->GetRoot();
  BrowserAccessibility* static_text_accessible =
      root_accessible->PlatformGetChild(0);

  EXPECT_EQ(gfx::Rect(100, 100, 60, 20).ToString(),
            static_text_accessible->GetLocalBoundsForRange(0, 6).ToString());

  EXPECT_EQ(gfx::Rect(100, 100, 10, 20).ToString(),
            static_text_accessible->GetLocalBoundsForRange(0, 1).ToString());

  EXPECT_EQ(gfx::Rect(100, 100, 30, 20).ToString(),
            static_text_accessible->GetLocalBoundsForRange(0, 3).ToString());

  EXPECT_EQ(gfx::Rect(150, 100, 10, 20).ToString(),
            static_text_accessible->GetLocalBoundsForRange(3, 1).ToString());

  EXPECT_EQ(gfx::Rect(130, 100, 30, 20).ToString(),
            static_text_accessible->GetLocalBoundsForRange(3, 3).ToString());

  // This range is only two characters, but because of the direction switch
  // the bounds are as wide as four characters.
  EXPECT_EQ(gfx::Rect(120, 100, 40, 20).ToString(),
            static_text_accessible->GetLocalBoundsForRange(2, 2).ToString());
}

}  // namespace content
