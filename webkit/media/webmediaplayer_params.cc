// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/media/webmediaplayer_params.h"

#include "base/message_loop_proxy.h"
#include "media/base/audio_renderer_sink.h"
#include "media/base/media_log.h"

namespace webkit_media {

WebMediaPlayerParams::WebMediaPlayerParams(
    const scoped_refptr<base::MessageLoopProxy>& message_loop_proxy,
    const scoped_refptr<media::AudioRendererSink>& audio_renderer_sink,
    const scoped_refptr<media::GpuVideoDecoder::Factories>& gpu_factories,
    const scoped_refptr<media::MediaLog>& media_log)
    : message_loop_proxy_(message_loop_proxy),
      audio_renderer_sink_(audio_renderer_sink),
      gpu_factories_(gpu_factories),
      media_log_(media_log) {
  DCHECK(message_loop_proxy);
  DCHECK(media_log_);
}

WebMediaPlayerParams::~WebMediaPlayerParams() {}

}  // namespace webkit_media
