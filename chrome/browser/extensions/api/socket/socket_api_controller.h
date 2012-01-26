// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_SOCKET_SOCKET_API_CONTROLLER_H_
#define CHROME_BROWSER_EXTENSIONS_API_SOCKET_SOCKET_API_CONTROLLER_H_
#pragma once

#include <string>
#include <map>

#include "base/memory/linked_ptr.h"
#include "base/memory/scoped_ptr.h"

class Profile;

namespace net {
class IPEndPoint;
}

namespace extensions {

// kSrcIdKey, or "srcId," binds a socket to the onEvent closure that was
// optionally passed to the socket.create() method. It's generated by us in
// schema_generated_bindings.js; the application code is unaware of it.
extern const char kSrcIdKey[];

class Socket;
class SocketEventNotifier;

// SocketController keeps track of a collection of Sockets and provides a
// convenient set of methods to manipulate them.
class SocketController {
 public:
  SocketController();
  virtual ~SocketController();

  // Create/Destroy are a pair. They represent the allocation and deallocation
  // of the Socket object in memory.
  //
  // TODO(miket): aa's suggestion to track lifetime of callbacks associated
  // with each socket, which will then let us clean up when we go out of scope
  // rather than requiring that the app developer remember to call Destroy.
  //
  // Takes ownership of |event_notifier|.
  int CreateTCPSocket(const std::string& address, int port,
                      SocketEventNotifier* event_notifier);
  int CreateUDPSocket(const std::string& address, int port,
                      SocketEventNotifier* event_notifier);

  bool DestroySocket(int socket_id);

  // Connect, Disconnect, Read, and Write map to the equivalent methods in the
  // underlying socket.
  int ConnectSocket(int socket_id);
  void DisconnectSocket(int socket_id);
  std::string ReadSocket(int socket_id);
  int WriteSocket(int socket_id, const std::string& message);

 private:
  int next_socket_id_;
  typedef std::map<int, linked_ptr<Socket> > SocketMap;
  SocketMap socket_map_;

  // Convenience method for accessing SocketMap.
  Socket* GetSocket(int socket_id);

  int GenerateSocketId();

  DISALLOW_COPY_AND_ASSIGN(SocketController);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_SOCKET_SOCKET_API_CONTROLLER_H_
