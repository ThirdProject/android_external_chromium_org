// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/frame_host/frame_tree.h"

#include <queue>

#include "base/bind.h"
#include "base/callback.h"
#include "content/browser/frame_host/frame_tree_node.h"
#include "content/browser/frame_host/navigator.h"
#include "content/browser/frame_host/render_frame_host_impl.h"

namespace content {

namespace {
// Used with FrameTree::ForEach() to search for the FrameTreeNode
// corresponding to |frame_tree_node_id|.
bool FrameTreeNodeForId(int64 frame_tree_node_id,
                        FrameTreeNode** out_node,
                        FrameTreeNode* node) {
  if (node->frame_tree_node_id() == frame_tree_node_id) {
    *out_node = node;
    // Terminate iteration once the node has been found.
    return false;
  }
  return true;
}

// TODO(creis): Remove this version along with FrameTreeNode::frame_id().
bool FrameTreeNodeForFrameId(int64 frame_id,
                             FrameTreeNode** out_node,
                             FrameTreeNode* node) {
  if (node->frame_id() == frame_id) {
    *out_node = node;
    // Terminate iteration once the node has been found.
    return false;
  }
  return true;
}

}  // namespace

FrameTree::FrameTree(Navigator* navigator)
    : root_(new FrameTreeNode(FrameTreeNode::kInvalidFrameId, std::string(),
                              navigator, scoped_ptr<RenderFrameHostImpl>())) {
}

FrameTree::~FrameTree() {
}

FrameTreeNode* FrameTree::FindByID(int64 frame_tree_node_id) {
  FrameTreeNode* node = NULL;
  ForEach(base::Bind(&FrameTreeNodeForId, frame_tree_node_id, &node));
  return node;
}

void FrameTree::ForEach(
    const base::Callback<bool(FrameTreeNode*)>& on_node) const {
  std::queue<FrameTreeNode*> queue;
  queue.push(root_.get());

  while (!queue.empty()) {
    FrameTreeNode* node = queue.front();
    queue.pop();
    if (!on_node.Run(node))
      break;

    for (size_t i = 0; i < node->child_count(); ++i)
      queue.push(node->child_at(i));
  }
}

bool FrameTree::IsFirstNavigationAfterSwap() const {
  return root_->frame_id() == FrameTreeNode::kInvalidFrameId;
}

void FrameTree::OnFirstNavigationAfterSwap(int main_frame_id) {
  root_->set_frame_id(main_frame_id);
}

void FrameTree::AddFrame(int render_frame_host_id,
                         int64 parent_frame_id,
                         int64 frame_id,
                         const std::string& frame_name) {
  FrameTreeNode* parent = FindByFrameID(parent_frame_id);
  // TODO(ajwong): Should the renderer be killed here? Would there be a race on
  // shutdown that might make this case possible?
  if (!parent)
    return;

  parent->AddChild(CreateNode(frame_id, frame_name, render_frame_host_id,
                              parent->navigator(),
                              parent->render_frame_host()->GetProcess()));
}

void FrameTree::RemoveFrame(int64 parent_frame_id, int64 frame_id) {
  // If switches::kSitePerProcess is not specified, then the FrameTree only
  // contains a node for the root element. However, even in this case
  // frame detachments need to be broadcast outwards.
  //
  // TODO(ajwong): Move this below the |parent| check after the FrameTree is
  // guaranteed to be correctly populated even without the
  // switches::kSitePerProcess flag.
  FrameTreeNode* parent = FindByFrameID(parent_frame_id);
  FrameTreeNode* child = FindByFrameID(frame_id);
  if (!on_frame_removed_.is_null()) {
    on_frame_removed_.Run(
        root_->render_frame_host()->render_view_host(), frame_id);
  }

  // TODO(ajwong): Should the renderer be killed here? Would there be a race on
  // shutdown that might make this case possible?
  if (!parent || !child)
    return;

  parent->RemoveChild(child);
}

void FrameTree::SetFrameUrl(int64 frame_id, const GURL& url) {
  FrameTreeNode* node = FindByFrameID(frame_id);
  // TODO(ajwong): Should the renderer be killed here? Would there be a race on
  // shutdown that might make this case possible?
  if (!node)
    return;

  if (node)
    node->set_current_url(url);
}

void FrameTree::SwapMainFrame(RenderFrameHostImpl* render_frame_host) {
  return root_->ResetForMainFrame(render_frame_host);
}

RenderFrameHostImpl* FrameTree::GetMainFrame() const {
  return root_->render_frame_host();
}

void FrameTree::SetFrameRemoveListener(
    const base::Callback<void(RenderViewHostImpl*, int64)>& on_frame_removed) {
  on_frame_removed_ = on_frame_removed;
}

FrameTreeNode* FrameTree::FindByFrameID(int64 frame_id) {
  FrameTreeNode* node = NULL;
  ForEach(base::Bind(&FrameTreeNodeForFrameId, frame_id, &node));
  return node;
}

scoped_ptr<FrameTreeNode> FrameTree::CreateNode(
    int64 frame_id,
    const std::string& frame_name,
    int render_frame_host_id,
    Navigator* navigator,
    RenderProcessHost* render_process_host) {
  scoped_ptr<RenderFrameHostImpl> render_frame_host(
      new RenderFrameHostImpl(root_->render_frame_host()->render_view_host(),
                              this, render_frame_host_id, false));

  return make_scoped_ptr(new FrameTreeNode(
      frame_id, frame_name, navigator, render_frame_host.Pass()));
}

}  // namespace content
