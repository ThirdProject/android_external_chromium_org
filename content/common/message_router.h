// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_MESSAGE_ROUTER_H_
#define CONTENT_COMMON_MESSAGE_ROUTER_H_

#include "base/id_map.h"
#include "ipc/ipc_listener.h"
#include "ipc/ipc_sender.h"

// The MessageRouter handles all incoming messages sent to it by routing them
// to the correct listener.  Routing is based on the Message's routing ID.
// Since routing IDs are typically assigned asynchronously by the browser
// process, the MessageRouter has the notion of pending IDs for listeners that
// have not yet been assigned a routing ID.
//
// When a message arrives, the routing ID is used to index the set of routes to
// find a listener.  If a listener is found, then the message is passed to it.
// Otherwise, the message is ignored if its routing ID is not equal to
// MSG_ROUTING_CONTROL.
//
// The MessageRouter supports the IPC::Sender interface for outgoing messages,
// but does not define a meaningful implementation of it.  The subclass of
// MessageRouter is intended to provide that if appropriate.
//
// The MessageRouter can be used as a concrete class provided its Send method
// is not called and it does not receive any control messages.

namespace content {

class MessageRouter : public IPC::Listener, public IPC::Sender {
 public:
  MessageRouter();
  virtual ~MessageRouter();

  // Implemented by subclasses to handle control messages
  virtual bool OnControlMessageReceived(const IPC::Message& msg);

  // IPC::Listener implementation:
  virtual bool OnMessageReceived(const IPC::Message& msg) OVERRIDE;

  // Like OnMessageReceived, except it only handles routed messages.  Returns
  // true if the message was dispatched, or false if there was no listener for
  // that route id.
  virtual bool RouteMessage(const IPC::Message& msg);

  // IPC::Sender implementation:
  virtual bool Send(IPC::Message* msg) OVERRIDE;

  // Called to add/remove a listener for a particular message routing ID.
  void AddRoute(int32 routing_id, IPC::Listener* listener);
  void RemoveRoute(int32 routing_id);

  IPC::Listener* ResolveRoute(int32 routing_id);

 private:
  // A list of all listeners with assigned routing IDs.
  IDMap<IPC::Listener> routes_;

  DISALLOW_COPY_AND_ASSIGN(MessageRouter);
};

}  // namespace content

#endif  // CONTENT_COMMON_MESSAGE_ROUTER_H_
