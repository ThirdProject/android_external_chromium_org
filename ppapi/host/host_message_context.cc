// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/host/host_message_context.h"

namespace ppapi {
namespace host {

ReplyMessageContext::ReplyMessageContext() {
}

ReplyMessageContext::ReplyMessageContext(
    const ppapi::proxy::ResourceMessageReplyParams& cp,
    IPC::Message* sync_reply_msg)
    : params(cp),
      sync_reply_msg(sync_reply_msg) {
}

ReplyMessageContext::~ReplyMessageContext() {
}

HostMessageContext::HostMessageContext(
    const ppapi::proxy::ResourceMessageCallParams& cp)
    : params(cp),
      sync_reply_msg(NULL) {
}

HostMessageContext::HostMessageContext(
    const ppapi::proxy::ResourceMessageCallParams& cp,
    IPC::Message* reply_msg)
    : params(cp),
      sync_reply_msg(reply_msg) {
}

HostMessageContext::~HostMessageContext() {
}

ReplyMessageContext HostMessageContext::MakeReplyMessageContext() const {
  ppapi::proxy::ResourceMessageReplyParams reply_params(params.pp_resource(),
                                                        params.sequence());
  return ReplyMessageContext(reply_params, sync_reply_msg);
}

}  // namespace host
}  // namespace ppapi
