// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/quic/quic_reliable_client_stream.h"

#include "net/base/net_errors.h"
#include "net/quic/quic_session.h"

namespace net {

QuicReliableClientStream::QuicReliableClientStream(QuicStreamId id,
                                                   QuicSession* session,
                                                   const BoundNetLog& net_log)
    : ReliableQuicStream(id, session),
      net_log_(net_log),
      delegate_(NULL) {
}

QuicReliableClientStream::~QuicReliableClientStream() {
  if (delegate_)
    delegate_->OnClose(error());
}

uint32 QuicReliableClientStream::ProcessData(const char* data,
                                             uint32 data_len) {
  // TODO(rch): buffer data if we don't have a delegate.
  if (!delegate_)
    return ERR_ABORTED;

  int rv = delegate_->OnDataReceived(data, data_len);
  if (rv != OK) {
    DLOG(ERROR) << "Delegate refused data, rv: " << rv;
    Close(QUIC_BAD_APPLICATION_PAYLOAD);
    return 0;
  }
  return data_len;
}

void QuicReliableClientStream::TerminateFromPeer(bool half_close) {
  if (delegate_) {
    delegate_->OnClose(error());
    delegate_ = NULL;
  }
  ReliableQuicStream::TerminateFromPeer(half_close);
}

void QuicReliableClientStream::SetDelegate(
    QuicReliableClientStream::Delegate* delegate) {
  DCHECK((!delegate_ && delegate) || (delegate_ && !delegate));
  delegate_ = delegate;
}

void QuicReliableClientStream::OnError(int error) {
  if (delegate_) {
    QuicReliableClientStream::Delegate* delegate = delegate_;
    delegate_ = NULL;
    delegate->OnError(error);
  }
}

}  // namespace net
