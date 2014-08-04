// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_WEBSOCKETS_WEBSOCKET_HANDSHAKE_STREAM_BASE_H_
#define NET_WEBSOCKETS_WEBSOCKET_HANDSHAKE_STREAM_BASE_H_

// This file is included from net/http files.
// Since net/http can be built without linking net/websockets code,
// this file must not introduce any link-time dependencies on websockets.

#include <string>

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/supports_user_data.h"
#include "net/http/http_stream_base.h"
#include "net/url_request/websocket_handshake_userdata_key.h"
#include "net/websockets/websocket_stream.h"

namespace net {

class ClientSocketHandle;
class SpdySession;

// WebSocketHandshakeStreamBase is the base class of
// WebSocketBasicHandshakeStream.  net/http code uses this interface to handle
// WebSocketBasicHandshakeStream when it needs to be treated differently from
// HttpStreamBase.
class NET_EXPORT WebSocketHandshakeStreamBase : public HttpStreamBase {
 public:
  // An object that stores data needed for the creation of a
  // WebSocketBasicHandshakeStream object. A new CreateHelper is used for each
  // WebSocket connection.
  class NET_EXPORT_PRIVATE CreateHelper : public base::SupportsUserData::Data {
   public:
    // Returns a key to use to lookup this object in a URLRequest object. It is
    // different from any other key that is supplied to
    // URLRequest::SetUserData().
    static const void* DataKey() { return kWebSocketHandshakeUserDataKey; }

    virtual ~CreateHelper() {}

    // Create a WebSocketBasicHandshakeStream. This is called after the
    // underlying connection has been established but before any handshake data
    // has been transferred. This can be called more than once in the case that
    // HTTP authentication is needed.
    virtual WebSocketHandshakeStreamBase* CreateBasicStream(
        scoped_ptr<ClientSocketHandle> connection,
        bool using_proxy) = 0;

    // Create a WebSocketSpdyHandshakeStream (unimplemented as of October 2013)
    virtual WebSocketHandshakeStreamBase* CreateSpdyStream(
        const base::WeakPtr<SpdySession>& session,
        bool use_relative_url) = 0;
  };

  // This has to have an inline implementation so that the net/url_request/
  // tests do not fail on iOS.
  virtual ~WebSocketHandshakeStreamBase() {}

  // After the handshake has completed, this method creates a WebSocketStream
  // (of the appropriate type) from the WebSocketHandshakeStreamBase object.
  // The WebSocketHandshakeStreamBase object is unusable after Upgrade() has
  // been called.
  virtual scoped_ptr<WebSocketStream> Upgrade() = 0;

 protected:
  // As with the destructor, this must be inline.
  WebSocketHandshakeStreamBase() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(WebSocketHandshakeStreamBase);
};

}  // namespace net

#endif  // NET_WEBSOCKETS_WEBSOCKET_HANDSHAKE_STREAM_BASE_H_
