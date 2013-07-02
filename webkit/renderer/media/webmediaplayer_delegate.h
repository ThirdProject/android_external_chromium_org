// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_RENDERER_MEDIA_WEBMEDIAPLAYER_DELEGATE_H_
#define WEBKIT_RENDERER_MEDIA_WEBMEDIAPLAYER_DELEGATE_H_

namespace WebKit {
class WebMediaPlayer;
}
namespace webkit_media {

// An interface to allow a WebMediaPlayerImpl to communicate changes of state
// to objects that need to know.
class WebMediaPlayerDelegate {
 public:
  WebMediaPlayerDelegate() {}

  // The specified player started playing media.
  virtual void DidPlay(WebKit::WebMediaPlayer* player) = 0;

  // The specified player stopped playing media.
  virtual void DidPause(WebKit::WebMediaPlayer* player) = 0;

  // The specified player was destroyed. Do not call any methods on it.
  virtual void PlayerGone(WebKit::WebMediaPlayer* player) = 0;

 protected:
  virtual ~WebMediaPlayerDelegate() {}
};

}  // namespace webkit_media

#endif  // WEBKIT_RENDERER_MEDIA_WEBMEDIAPLAYER_DELEGATE_H_
