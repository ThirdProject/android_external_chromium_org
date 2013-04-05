// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_CPP_VIDEO_WRITER_H_
#define PPAPI_CPP_VIDEO_WRITER_H_

#include <string>

#include "ppapi/c/pp_time.h"
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/pass_ref.h"
#include "ppapi/cpp/resource.h"

/// @file
/// This file defines the API to create and use video stream readers and
/// writers.

namespace pp {

class InstanceHandle;
class VideoFrame;

// VideoWriter -----------------------------------------------------------------

/// The <code>VideoWriter</code> class represents a video writer resource.
class VideoWriter : public Resource {
 public:
  /// Default constructor for creating a <code>VideoWriter</code> object.
  VideoWriter();

  /// Constructor for creating a <code>VideoWriter</code> for an instance.
  explicit VideoWriter(const InstanceHandle& instance);

  /// The copy constructor for <code>VideoWriter</code>.
  ///
  /// @param[in] other A reference to a <code>VideoWriter</code>.
  VideoWriter(const VideoWriter& other);

  /// A constructor used when you have received a PP_Resource as a return
  /// value that has had its reference count incremented for you.
  ///
  /// @param[in] resource A PP_Resource corresponding to a video writer.
  VideoWriter(PassRef, PP_Resource resource);

  /// Opens a stream for writing video and associates it with the given id.
  ///
  /// @param[in] stream_id A string uniquely identifying the stream to write to.
  /// @param[in] callback A <code>CompletionCallback</code> to be called upon
  /// completion of Open.
  ///
  /// @return A return code from <code>pp_errors.h</code>.
  int32_t Open(const std::string& stream_id,
               const CompletionCallback& cc);

  /// Puts the next frame of video to the writer's stream.
  ///
  /// @param[in] frame A <code>VideoFrame</code> containing the frame to write
  /// to the open stream.
  ///
  /// @return A return code from <code>pp_errors.h</code>.
  int32_t PutFrame(const VideoFrame& frame);

  /// Closes the writer's current stream.
  void Close();
};

}  // namespace pp

#endif  // PPAPI_CPP_VIDEO_WRITER_H_
