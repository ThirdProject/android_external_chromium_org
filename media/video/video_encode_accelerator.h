// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_VIDEO_VIDEO_ENCODE_ACCELERATOR_H_
#define MEDIA_VIDEO_VIDEO_ENCODE_ACCELERATOR_H_

#include <vector>

#include "base/basictypes.h"
#include "base/memory/ref_counted.h"
#include "media/base/bitstream_buffer.h"
#include "media/base/media_export.h"
#include "media/base/video_decoder_config.h"
#include "media/base/video_frame.h"

namespace media {

class BitstreamBuffer;
class VideoFrame;

// Video encoder interface.
class MEDIA_EXPORT VideoEncodeAccelerator {
 public:
  virtual ~VideoEncodeAccelerator();

  // Specification of an encoding profile supported by an encoder.
  struct SupportedProfile {
    VideoCodecProfile profile;
    gfx::Size max_resolution;
    struct {
      uint32 numerator;
      uint32 denominator;
    } max_framerate;
  };

  // Enumeration of potential errors generated by the API.
  enum Error {
    // An operation was attempted during an incompatible encoder state.
    kIllegalStateError,
    // Invalid argument was passed to an API method.
    kInvalidArgumentError,
    // A failure occurred at the GPU process or one of its dependencies.
    // Examples of such failures include GPU hardware failures, GPU driver
    // failures, GPU library failures, GPU process programming errors, and so
    // on.
    kPlatformFailureError,
    kErrorMax = kPlatformFailureError
  };

  // Interface for clients that use VideoEncodeAccelerator.
  class MEDIA_EXPORT Client {
   public:
    // Callback to notify client that encoder has been successfully initialized.
    virtual void NotifyInitializeDone() = 0;

    // Callback to tell the client what size of frames and buffers to provide
    // for input and output.  The VEA disclaims use or ownership of all
    // previously provided buffers once this callback is made.
    // Parameters:
    //  |input_count| is the number of input VideoFrames required for encoding.
    //  The client should be prepared to feed at least this many frames into the
    //  encoder before being returned any input frames, since the encoder may
    //  need to hold onto some subset of inputs as reference pictures.
    //  |input_coded_size| is the logical size of the input frames (as reported
    //  by VideoFrame::coded_size()) to encode, in pixels.  The encoder may have
    //  hardware alignment requirements that make this different from
    //  |input_visible_size|, as requested in Initialize(), in which case the
    //  input VideoFrame to Encode() should be padded appropriately.
    //  |output_buffer_size| is the required size of output buffers for this
    //  encoder in bytes.
    virtual void RequireBitstreamBuffers(unsigned int input_count,
                                         const gfx::Size& input_coded_size,
                                         size_t output_buffer_size) = 0;

    // Callback to deliver encoded bitstream buffers.  Ownership of the buffer
    // is transferred back to the VEA::Client once this callback is made.
    // Parameters:
    //  |bitstream_buffer_id| is the id of the buffer that is ready.
    //  |payload_size| is the byte size of the used portion of the buffer.
    //  |key_frame| is true if this delivered frame is a keyframe.
    virtual void BitstreamBufferReady(int32 bitstream_buffer_id,
                                      size_t payload_size,
                                      bool key_frame) = 0;

    // Error notification callback.
    virtual void NotifyError(Error error) = 0;

   protected:
    // Clients are not owned by VEA instances and should not be deleted through
    // these pointers.
    virtual ~Client() {}
  };

  // Video encoder functions.

  // Initialize the video encoder with a specific configuration.  Called once
  // per encoder construction.
  // Parameters:
  //  |input_format| is the frame format of the input stream (as would be
  //  reported by VideoFrame::format() for frames passed to Encode()).
  //  |input_visible_size| is the resolution of the input stream (as would be
  //  reported by VideoFrame::visible_rect().size() for frames passed to
  //  Encode()).
  //  |output_profile| is the codec profile of the encoded output stream.
  //  |initial_bitrate| is the initial bitrate of the encoded output stream,
  //  in bits per second.
  //  |client| is the client of this video encoder.  The provided pointer must
  //  be valid until Destroy() is called.
  // TODO(sheu): handle resolution changes.  http://crbug.com/249944
  virtual void Initialize(VideoFrame::Format input_format,
                          const gfx::Size& input_visible_size,
                          VideoCodecProfile output_profile,
                          uint32 initial_bitrate,
                          Client* client) = 0;

  // Encodes the given frame.
  // Parameters:
  //  |frame| is the VideoFrame that is to be encoded.
  //  |force_keyframe| forces the encoding of a keyframe for this frame.
  virtual void Encode(const scoped_refptr<VideoFrame>& frame,
                      bool force_keyframe) = 0;

  // Send a bitstream buffer to the encoder to be used for storing future
  // encoded output.  Each call here with a given |buffer| will cause the buffer
  // to be filled once, then returned with BitstreamBufferReady().
  // Parameters:
  //  |buffer| is the bitstream buffer to use for output.
  virtual void UseOutputBitstreamBuffer(const BitstreamBuffer& buffer) = 0;

  // Request a change to the encoding parameters.  This is only a request,
  // fulfilled on a best-effort basis.
  // Parameters:
  //  |bitrate| is the requested new bitrate, in bits per second.
  //  |framerate| is the requested new framerate, in frames per second.
  virtual void RequestEncodingParametersChange(uint32 bitrate,
                                               uint32 framerate) = 0;

  // Destroys the encoder: all pending inputs and outputs are dropped
  // immediately and the component is freed.  This call may asynchronously free
  // system resources, but its client-visible effects are synchronous.  After
  // this method returns no more callbacks will be made on the client.  Deletes
  // |this| unconditionally, so make sure to drop all pointers to it!
  virtual void Destroy() = 0;
};

}  // namespace media

#endif  // MEDIA_VIDEO_VIDEO_ENCODE_ACCELERATOR_H_
