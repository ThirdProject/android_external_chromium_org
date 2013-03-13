// Copyright 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_VIDEO_LAYER_H_
#define CC_VIDEO_LAYER_H_

#include "base/callback.h"
#include "cc/cc_export.h"
#include "cc/layer.h"

namespace media { class VideoFrame; }

namespace cc {

class VideoFrameProvider;
class VideoLayerImpl;

// A Layer that contains a Video element.
class CC_EXPORT VideoLayer : public Layer {
 public:
  static scoped_refptr<VideoLayer> Create(VideoFrameProvider* provider);

  virtual scoped_ptr<LayerImpl> CreateLayerImpl(LayerTreeImpl* tree_impl)
      OVERRIDE;

 private:
  explicit VideoLayer(VideoFrameProvider* provider);
  virtual ~VideoLayer();

  // This pointer is only for passing to VideoLayerImpl's constructor. It should
  // never be dereferenced by this class.
  VideoFrameProvider* provider_;
};

}  // namespace cc

#endif  // CC_VIDEO_LAYER_H_
