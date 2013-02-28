// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/delegated_renderer_layer.h"

#include "cc/delegated_frame_data.h"
#include "cc/delegated_renderer_layer_impl.h"

namespace cc {

scoped_refptr<DelegatedRendererLayer> DelegatedRendererLayer::Create() {
  return scoped_refptr<DelegatedRendererLayer>(new DelegatedRendererLayer());
}

DelegatedRendererLayer::DelegatedRendererLayer()
    : Layer() {
}

DelegatedRendererLayer::~DelegatedRendererLayer() {}

scoped_ptr<LayerImpl> DelegatedRendererLayer::createLayerImpl(
    LayerTreeImpl* tree_impl) {
  return DelegatedRendererLayerImpl::Create(
      tree_impl, m_layerId).PassAs<LayerImpl>();
}

bool DelegatedRendererLayer::drawsContent() const {
  return !frame_size_.IsEmpty();
}

void DelegatedRendererLayer::pushPropertiesTo(LayerImpl* impl) {
  Layer::pushPropertiesTo(impl);

  DelegatedRendererLayerImpl* delegated_impl =
      static_cast<DelegatedRendererLayerImpl*>(impl);

  delegated_impl->SetDisplaySize(display_size_);

  if (frame_data_) {
    if (frame_size_.IsEmpty()) {
      scoped_ptr<DelegatedFrameData> empty_frame(new DelegatedFrameData);
      delegated_impl->SetFrameData(empty_frame.Pass(), gfx::Rect());
    } else {
      delegated_impl->SetFrameData(frame_data_.Pass(), damage_in_frame_);
    }
    frame_data_.reset();
    damage_in_frame_ = gfx::RectF();
  }
}

void DelegatedRendererLayer::SetDisplaySize(gfx::Size size) {
  if (display_size_ == size)
    return;
  display_size_ = size;
  setNeedsCommit();
}

void DelegatedRendererLayer::SetFrameData(
    scoped_ptr<DelegatedFrameData> new_frame_data) {
  frame_data_ = new_frame_data.Pass();
  if (!frame_data_->render_pass_list.empty()) {
    RenderPass* root_pass = frame_data_->render_pass_list.back();
    damage_in_frame_.Union(root_pass->damage_rect);
    frame_size_ = root_pass->output_rect.size();
  } else {
    frame_size_ = gfx::Size();
  }
  setNeedsCommit();
}

}  // namespace cc
