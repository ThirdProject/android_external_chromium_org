// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_HISTOGRAM_MESSAGE_FILTER_H_
#define CONTENT_BROWSER_HISTOGRAM_MESSAGE_FILTER_H_

#include <string>
#include <vector>

#include "content/public/browser/browser_message_filter.h"
#include "content/public/common/process_type.h"

namespace content {

// This class sends and receives histogram messages in the browser process.
class HistogramMessageFilter : public BrowserMessageFilter {
 public:
  HistogramMessageFilter();

  // content::BrowserMessageFilter implementation.
  virtual void OnChannelConnected(int32 peer_pid) OVERRIDE;

  // content::BrowserMessageFilter implementation.
  virtual bool OnMessageReceived(const IPC::Message& message,
                                 bool* message_was_ok) OVERRIDE;

 private:
  virtual ~HistogramMessageFilter();

  // Message handlers.
  void OnChildHistogramData(int sequence_number,
                            const std::vector<std::string>& pickled_histograms);

  ProcessType process_type_;

  DISALLOW_COPY_AND_ASSIGN(HistogramMessageFilter);
};

}  // namespace content

#endif  // CONTENT_BROWSER_HISTOGRAM_MESSAGE_FILTER_H_
