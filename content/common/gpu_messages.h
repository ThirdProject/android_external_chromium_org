// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>
#include <string>


#include "base/shared_memory.h"
#include "content/common/common_param_traits.h"
#include "content/common/gpu/gpu_info.h"
#include "content/common/gpu_process_launch_causes.h"
#include "gpu/command_buffer/common/command_buffer.h"
#include "gpu/ipc/gpu_command_buffer_traits.h"
#include "ipc/ipc_channel_handle.h"
#include "ipc/ipc_message_macros.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/size.h"
// Multiply-included message file, hence no include guard.

#define IPC_MESSAGE_START GpuMsgStart

// Flag assigned to a video buffer for both input and output.
#define kGpuVideoEndOfStream (1 << 0)

IPC_STRUCT_BEGIN(GPUCreateCommandBufferConfig)
  IPC_STRUCT_MEMBER(std::string, allowed_extensions)
  IPC_STRUCT_MEMBER(std::vector<int>, attribs)
  IPC_STRUCT_MEMBER(GURL, active_url)
IPC_STRUCT_END()

IPC_STRUCT_BEGIN(GpuVideoDecoderInitParam)
  IPC_STRUCT_MEMBER(int32, codec_id)
  IPC_STRUCT_MEMBER(int32, width)
  IPC_STRUCT_MEMBER(int32, height)
  IPC_STRUCT_MEMBER(int32, profile)
  IPC_STRUCT_MEMBER(int32, level)
  IPC_STRUCT_MEMBER(int32, frame_rate_den)
  IPC_STRUCT_MEMBER(int32, frame_rate_num)
  IPC_STRUCT_MEMBER(int32, aspect_ratio_den)
  IPC_STRUCT_MEMBER(int32, aspect_ratio_num)
IPC_STRUCT_END()

IPC_STRUCT_BEGIN(GpuVideoDecoderInitDoneParam)
  // other parameter is only meaningful when this is true.
  IPC_STRUCT_MEMBER(int32, success)
  IPC_STRUCT_MEMBER(int32, input_buffer_size)
  IPC_STRUCT_MEMBER(base::SharedMemoryHandle, input_buffer_handle)
IPC_STRUCT_END()

IPC_STRUCT_BEGIN(GpuVideoDecoderInputBufferParam)
  IPC_STRUCT_MEMBER(int64, timestamp)  // In unit of microseconds.
  IPC_STRUCT_MEMBER(int32, offset)
  IPC_STRUCT_MEMBER(int32, size)
  IPC_STRUCT_MEMBER(int32, flags)  // Miscellaneous flag bit mask.
IPC_STRUCT_END()

IPC_STRUCT_BEGIN(GpuVideoDecoderErrorInfoParam)
  IPC_STRUCT_MEMBER(int32, error_id)  // TODO(jiesun): define enum.
IPC_STRUCT_END()

// TODO(jiesun): define this.
IPC_STRUCT_BEGIN(GpuVideoDecoderFormatChangeParam)
  IPC_STRUCT_MEMBER(int32, input_buffer_size)
  IPC_STRUCT_MEMBER(base::SharedMemoryHandle, input_buffer_handle)
IPC_STRUCT_END()

IPC_STRUCT_BEGIN(AcceleratedVideoDecoderDecodeParam)
  IPC_STRUCT_MEMBER(base::SharedMemoryHandle, input_buffer_handle)
  IPC_STRUCT_MEMBER(int32, offset)
  IPC_STRUCT_MEMBER(int32, size)
  IPC_STRUCT_MEMBER(int32, flags)  // Miscellaneous flag bit mask.
IPC_STRUCT_END()

#if defined(OS_MACOSX)
IPC_STRUCT_BEGIN(GpuHostMsg_AcceleratedSurfaceSetIOSurface_Params)
  IPC_STRUCT_MEMBER(int32, renderer_id)
  IPC_STRUCT_MEMBER(int32, render_view_id)
  IPC_STRUCT_MEMBER(gfx::PluginWindowHandle, window)
  IPC_STRUCT_MEMBER(int32, width)
  IPC_STRUCT_MEMBER(int32, height)
  IPC_STRUCT_MEMBER(uint64, identifier)
IPC_STRUCT_END()

IPC_STRUCT_BEGIN(GpuHostMsg_AcceleratedSurfaceBuffersSwapped_Params)
  IPC_STRUCT_MEMBER(int32, renderer_id)
  IPC_STRUCT_MEMBER(int32, render_view_id)
  IPC_STRUCT_MEMBER(gfx::PluginWindowHandle, window)
  IPC_STRUCT_MEMBER(uint64, surface_id)
  IPC_STRUCT_MEMBER(int32, route_id)
  IPC_STRUCT_MEMBER(uint64, swap_buffers_count)
IPC_STRUCT_END()
#endif

IPC_STRUCT_TRAITS_BEGIN(DxDiagNode)
  IPC_STRUCT_TRAITS_MEMBER(values)
  IPC_STRUCT_TRAITS_MEMBER(children)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(GPUInfo)
  IPC_STRUCT_TRAITS_MEMBER(finalized)
  IPC_STRUCT_TRAITS_MEMBER(initialization_time)
  IPC_STRUCT_TRAITS_MEMBER(vendor_id)
  IPC_STRUCT_TRAITS_MEMBER(device_id)
  IPC_STRUCT_TRAITS_MEMBER(driver_vendor)
  IPC_STRUCT_TRAITS_MEMBER(driver_version)
  IPC_STRUCT_TRAITS_MEMBER(driver_date)
  IPC_STRUCT_TRAITS_MEMBER(pixel_shader_version)
  IPC_STRUCT_TRAITS_MEMBER(vertex_shader_version)
  IPC_STRUCT_TRAITS_MEMBER(gl_version)
  IPC_STRUCT_TRAITS_MEMBER(gl_version_string)
  IPC_STRUCT_TRAITS_MEMBER(gl_vendor)
  IPC_STRUCT_TRAITS_MEMBER(gl_renderer)
  IPC_STRUCT_TRAITS_MEMBER(gl_extensions)
  IPC_STRUCT_TRAITS_MEMBER(can_lose_context)
#if defined(OS_WIN)
  IPC_STRUCT_TRAITS_MEMBER(dx_diagnostics)
#endif
IPC_STRUCT_TRAITS_END()

IPC_ENUM_TRAITS(content::CauseForGpuLaunch)

//------------------------------------------------------------------------------
// GPU Messages
// These are messages from the browser to the GPU process.

// Tells the GPU process to initialize itself. The browser explicitly
// requests this be done so that we are guaranteed that the channel is set
// up between the browser and GPU process before doing any work that might
// potentially crash the GPU process. Detection of the child process
// exiting abruptly is predicated on having the IPC channel set up.
IPC_MESSAGE_CONTROL0(GpuMsg_Initialize)

// Tells the GPU process to create a new channel for communication with a
// given renderer.  The channel name is returned in a
// GpuHostMsg_ChannelEstablished message.  The renderer ID is passed so that
// the GPU process reuses an existing channel to that process if it exists.
// This ID is a unique opaque identifier generated by the browser process.
IPC_MESSAGE_CONTROL1(GpuMsg_EstablishChannel,
                     int /* renderer_id */)

// Tells the GPU process to close the channel identified by IPC channel
// handle.  If no channel can be identified, do nothing.
IPC_MESSAGE_CONTROL1(GpuMsg_CloseChannel,
                     IPC::ChannelHandle /* channel_handle */)

// Provides a synchronization point to guarantee that the processing of
// previous asynchronous messages (i.e., GpuMsg_EstablishChannel) has
// completed. (This message can't be synchronous because the
// GpuProcessHost uses an IPC::ChannelProxy, which sends all messages
// asynchronously.) Results in a GpuHostMsg_SynchronizeReply.
IPC_MESSAGE_CONTROL0(GpuMsg_Synchronize)

// Tells the GPU process to create a new command buffer that renders directly
// to a native view. A corresponding GpuCommandBufferStub is created.
IPC_MESSAGE_CONTROL4(GpuMsg_CreateViewCommandBuffer,
                     gfx::PluginWindowHandle, /* view */
                     int32, /* render_view_id */
                     int32, /* renderer_id */
                     GPUCreateCommandBufferConfig /* init_params */)

// Tells the GPU process to create a context for collecting graphics card
// information.
IPC_MESSAGE_CONTROL0(GpuMsg_CollectGraphicsInfo)

#if defined(OS_MACOSX)
// Tells the GPU process that the browser process handled the swap
// buffers request with the given number. Note that it is possible
// for the browser process to coalesce frames; it is not guaranteed
// that every GpuHostMsg_AcceleratedSurfaceBuffersSwapped message
// will result in a buffer swap on the browser side.
IPC_MESSAGE_CONTROL3(GpuMsg_AcceleratedSurfaceBuffersSwappedACK,
                     int /* renderer_id */,
                     int32 /* route_id */,
                     uint64 /* swap_buffers_count */)

// Requests the GPU process to destroy the command buffer and remove the
// associated route. Further messages to this command buffer will result in a
// channel error.
IPC_MESSAGE_CONTROL2(GpuMsg_DestroyCommandBuffer,
                     int /* renderer_id */,
                     int32 /* render_view_id */)
#endif

// Tells the GPU process to crash.
IPC_MESSAGE_CONTROL0(GpuMsg_Crash)

// Tells the GPU process to hang.
IPC_MESSAGE_CONTROL0(GpuMsg_Hang)

// The browser sends this to a renderer process in response to a
// GpuHostMsg_EstablishGpuChannel message.
IPC_MESSAGE_CONTROL3(GpuMsg_GpuChannelEstablished,
                     IPC::ChannelHandle /* handle to channel */,
                     base::ProcessHandle /* renderer_process_for_gpu */,
                     GPUInfo /* stats about GPU process*/)

//------------------------------------------------------------------------------
// GPU Host Messages
// These are messages to the browser.

// A renderer sends this when it wants to create a connection to the GPU
// process. The browser will create the GPU process if necessary, and will
// return a handle to the channel via a GpuChannelEstablished message.
IPC_MESSAGE_CONTROL1(GpuHostMsg_EstablishGpuChannel,
                     content::CauseForGpuLaunch)

// A renderer sends this to the browser process to provide a synchronization
// point for GPU operations, in particular to make sure the GPU channel has
// been established.
IPC_SYNC_MESSAGE_CONTROL0_0(GpuHostMsg_SynchronizeGpu)

// A renderer sends this to the browser process when it wants to
// create a GL context associated with the given view_id.
IPC_SYNC_MESSAGE_CONTROL2_1(GpuHostMsg_CreateViewCommandBuffer,
                            int32, /* render_view_id */
                            GPUCreateCommandBufferConfig, /* init_params */
                            int32 /* route_id */)

// Response from GPU to a GpuHostMsg_EstablishChannel message.
IPC_MESSAGE_CONTROL1(GpuHostMsg_ChannelEstablished,
                     IPC::ChannelHandle /* channel_handle */)

// Respond from GPU to a GpuMsg_CreateViewCommandBuffer message.
IPC_MESSAGE_CONTROL1(GpuHostMsg_CommandBufferCreated,
                     int32 /* route_id */)

// Request from GPU to free the browser resources associated with the
// command buffer.
IPC_MESSAGE_CONTROL3(GpuHostMsg_DestroyCommandBuffer,
                     gfx::PluginWindowHandle, /* view */
                     int32, /* render_view_id */
                     int32 /* renderer_id */)

// Response from GPU to a GpuMsg_CollectGraphicsInfo.
IPC_MESSAGE_CONTROL1(GpuHostMsg_GraphicsInfoCollected,
                     GPUInfo /* GPU logging stats */)

// Message from GPU to add a GPU log message to the about:gpu page.
IPC_MESSAGE_CONTROL3(GpuHostMsg_OnLogMessage,
                     int /*severity*/,
                     std::string /* header */,
                     std::string /* message */)

// Response from GPU to a GpuMsg_Synchronize message.
IPC_MESSAGE_CONTROL0(GpuHostMsg_SynchronizeReply)

#if defined(OS_LINUX) && !defined(TOUCH_UI)
// Resize the window that is being drawn into. It's important that this
// resize be synchronized with the swapping of the front and back buffers.
IPC_SYNC_MESSAGE_CONTROL2_1(GpuHostMsg_ResizeXID,
                            unsigned long, /* xid */
                            gfx::Size, /* size */
                            bool /* success */)
#elif defined(OS_MACOSX)
// This message, used on Mac OS X 10.6 and later (where IOSurface is
// supported), is sent from the GPU process to the browser to indicate that a
// new backing store was allocated for the given "window" (fake
// PluginWindowHandle). The renderer ID and render view ID are needed in
// order to uniquely identify the RenderWidgetHostView on the browser side.
IPC_MESSAGE_CONTROL1(GpuHostMsg_AcceleratedSurfaceSetIOSurface,
                     GpuHostMsg_AcceleratedSurfaceSetIOSurface_Params)

// This message notifies the browser process that the renderer
// swapped the buffers associated with the given "window", which
// should cause the browser to redraw the compositor's contents.
IPC_MESSAGE_CONTROL1(GpuHostMsg_AcceleratedSurfaceBuffersSwapped,
                     GpuHostMsg_AcceleratedSurfaceBuffersSwapped_Params)
#elif defined(OS_WIN)
IPC_MESSAGE_CONTROL2(GpuHostMsg_ScheduleComposite,
                     int32, /* renderer_id */
                     int32 /* render_view_id */)
#endif

//------------------------------------------------------------------------------
// GPU Channel Messages
// These are messages from a renderer process to the GPU process.

// Initialize a channel between a renderer process and a GPU process. The
// renderer passes its process handle to the GPU process, which gives gives the
// GPU process the ability to map handles from the renderer process. This must
// be the first message sent on a newly connected channel.
IPC_MESSAGE_CONTROL1(GpuChannelMsg_Initialize,
                     base::ProcessHandle /* renderer_process_for_gpu */)

// Tells the GPU process to create a new command buffer that renders to an
// offscreen frame buffer. If parent_route_id is not zero, the texture backing
// the frame buffer is mapped into the corresponding parent command buffer's
// namespace, with the name of parent_texture_id. This ID is in the parent's
// namespace.
IPC_SYNC_MESSAGE_CONTROL4_1(GpuChannelMsg_CreateOffscreenCommandBuffer,
                            int32, /* parent_route_id */
                            gfx::Size, /* size */
                            GPUCreateCommandBufferConfig, /* init_params */
                            uint32, /* parent_texture_id */
                            int32 /* route_id */)

// The CommandBufferProxy sends this to the GpuCommandBufferStub in its
// destructor, so that the stub deletes the actual CommandBufferService
// object that it's hosting.
// TODO(apatrick): Implement this.
IPC_SYNC_MESSAGE_CONTROL1_0(GpuChannelMsg_DestroyCommandBuffer,
                            int32 /* instance_id */)

// Create hardware video decoder && associate it with the output |decoder_id|;
// We need this to be control message because we had to map the GpuChannel and
// |decoder_id|.
IPC_MESSAGE_CONTROL2(GpuChannelMsg_CreateVideoDecoder,
                     int32, /* context_route_id */
                     int32) /* decoder_id */

// Release all resource of the hardware video decoder which was assocaited
// with the input |decoder_id|.
// TODO(hclam): This message needs to be asynchronous.
IPC_SYNC_MESSAGE_CONTROL1_0(GpuChannelMsg_DestroyVideoDecoder,
                            int32 /* decoder_id */)

// Create a TransportTexture corresponding to |host_id|.
IPC_MESSAGE_CONTROL2(GpuChannelMsg_CreateTransportTexture,
                     int32, /* context_route_id */
                     int32 /* host_id */)

//------------------------------------------------------------------------------
// GPU Command Buffer Messages
// These are messages between a renderer process to the GPU process relating to
// a single OpenGL context.
// Initialize a command buffer with the given number of command entries.
// Returns the shared memory handle for the command buffer mapped to the
// calling process.
IPC_SYNC_MESSAGE_ROUTED2_1(GpuCommandBufferMsg_Initialize,
                           base::SharedMemoryHandle /* ring_buffer */,
                           int32 /* size */,
                           bool /* result */)

// Get the current state of the command buffer.
IPC_SYNC_MESSAGE_ROUTED0_1(GpuCommandBufferMsg_GetState,
                           gpu::CommandBuffer::State /* state */)

// Get the current state of the command buffer asynchronously. State is
// returned via UpdateState message.
IPC_MESSAGE_ROUTED0(GpuCommandBufferMsg_AsyncGetState)

// Synchronize the put and get offsets of both processes. Caller passes its
// current put offset. Current state (including get offset) is returned.
IPC_SYNC_MESSAGE_ROUTED1_1(GpuCommandBufferMsg_Flush,
                           int32 /* put_offset */,
                           gpu::CommandBuffer::State /* state */)

// Asynchronously synchronize the put and get offsets of both processes.
// Caller passes its current put offset. Current state (including get offset)
// is returned via an UpdateState message.
IPC_MESSAGE_ROUTED1(GpuCommandBufferMsg_AsyncFlush,
                    int32 /* put_offset */)

// Return the current state of the command buffer following a request via
// an AsyncGetState or AsyncFlush message. (This message is sent from the
// GPU process to the renderer process.)
IPC_MESSAGE_ROUTED1(GpuCommandBufferMsg_UpdateState,
                    gpu::CommandBuffer::State /* state */)

// Indicates that a SwapBuffers call has been issued.
IPC_MESSAGE_ROUTED0(GpuCommandBufferMsg_SwapBuffers)

// Create a shared memory transfer buffer. Returns an id that can be used to
// identify the transfer buffer from a comment.
IPC_SYNC_MESSAGE_ROUTED1_1(GpuCommandBufferMsg_CreateTransferBuffer,
                           int32 /* size */,
                           int32 /* id */)

// Register an existing shared memory transfer buffer. Returns an id that can be
// used to identify the transfer buffer from a command buffer.
IPC_SYNC_MESSAGE_ROUTED2_1(GpuCommandBufferMsg_RegisterTransferBuffer,
                           base::SharedMemoryHandle /* transfer_buffer */,
                           size_t /* size */,
                           int32 /* id */)

// Destroy a previously created transfer buffer.
IPC_SYNC_MESSAGE_ROUTED1_0(GpuCommandBufferMsg_DestroyTransferBuffer,
                           int32 /* id */)

// Get the shared memory handle for a transfer buffer mapped to the callers
// process.
IPC_SYNC_MESSAGE_ROUTED1_2(GpuCommandBufferMsg_GetTransferBuffer,
                           int32 /* id */,
                           base::SharedMemoryHandle /* transfer_buffer */,
                           uint32 /* size */)

// Send from command buffer stub to proxy when window is invalid and must be
// repainted.
IPC_MESSAGE_ROUTED0(GpuCommandBufferMsg_NotifyRepaint)

// Tells the GPU process to resize an offscreen frame buffer.
IPC_MESSAGE_ROUTED1(GpuCommandBufferMsg_ResizeOffscreenFrameBuffer,
                    gfx::Size /* size */)

#if defined(OS_MACOSX)
// On Mac OS X the GPU plugin must be offscreen, because there is no
// true cross-process window hierarchy. For this reason we must send
// resize events explicitly to the command buffer stub so it can
// reallocate its backing store and send the new one back to the
// browser. This message is currently used only on 10.6 and later.
IPC_MESSAGE_ROUTED1(GpuCommandBufferMsg_SetWindowSize,
                    gfx::Size /* size */)
#endif

//------------------------------------------------------------------------------
// GPU Video Decoder Messages
// These messages are sent from Renderer process to GPU process.
// Initialize and configure GpuVideoDecoder asynchronously.
IPC_MESSAGE_ROUTED1(GpuVideoDecoderMsg_Initialize,
                    GpuVideoDecoderInitParam)

// Destroy and release GpuVideoDecoder asynchronously.
IPC_MESSAGE_ROUTED0(GpuVideoDecoderMsg_Destroy)

// Start decoder flushing operation.
IPC_MESSAGE_ROUTED0(GpuVideoDecoderMsg_Flush)

// Tell the decoder to start prerolling.
IPC_MESSAGE_ROUTED0(GpuVideoDecoderMsg_Preroll)

// Send input buffer to GpuVideoDecoder.
IPC_MESSAGE_ROUTED1(GpuVideoDecoderMsg_EmptyThisBuffer,
                    GpuVideoDecoderInputBufferParam)

// Ask the GPU process to produce a video frame with the ID.
IPC_MESSAGE_ROUTED1(GpuVideoDecoderMsg_ProduceVideoFrame,
                    int32) /* Video Frame ID */

// Sent from Renderer process to the GPU process to notify that textures are
// generated for a video frame.
IPC_MESSAGE_ROUTED2(GpuVideoDecoderMsg_VideoFrameAllocated,
                    int32, /* Video Frame ID */
                    std::vector<uint32>) /* Textures for video frame */

//------------------------------------------------------------------------------
// GPU Video Decoder Host Messages
// These messages are sent from GPU process to Renderer process.
// Inform GpuVideoDecoderHost that a GpuVideoDecoder is created.
IPC_MESSAGE_ROUTED1(GpuVideoDecoderHostMsg_CreateVideoDecoderDone,
                    int32) /* decoder_id */

// Confirm GpuVideoDecoder had been initialized or failed to initialize.
// TODO(hclam): Change this to Done instead of ACK.
IPC_MESSAGE_ROUTED1(GpuVideoDecoderHostMsg_InitializeACK,
                    GpuVideoDecoderInitDoneParam)

// Confrim GpuVideoDecoder had been destroyed properly.
// TODO(hclam): Change this to Done instead of ACK.
IPC_MESSAGE_ROUTED0(GpuVideoDecoderHostMsg_DestroyACK)

// Confirm decoder had been flushed.
// TODO(hclam): Change this to Done instead of ACK.
IPC_MESSAGE_ROUTED0(GpuVideoDecoderHostMsg_FlushACK)

// Confirm preroll operation is done.
IPC_MESSAGE_ROUTED0(GpuVideoDecoderHostMsg_PrerollDone)

// GpuVideoDecoder has consumed input buffer from transfer buffer.
// TODO(hclam): Change this to Done instead of ACK.
IPC_MESSAGE_ROUTED0(GpuVideoDecoderHostMsg_EmptyThisBufferACK)

// GpuVideoDecoder require new input buffer.
IPC_MESSAGE_ROUTED0(GpuVideoDecoderHostMsg_EmptyThisBufferDone)

// GpuVideoDecoder reports that a video frame is ready to be consumed.
IPC_MESSAGE_ROUTED4(GpuVideoDecoderHostMsg_ConsumeVideoFrame,
                    int32, /* Video Frame ID */
                    int64, /* Timestamp in microseconds */
                    int64, /* Duration in microseconds */
                    int32) /* Flags */

// Allocate video frames for output of the hardware video decoder.
IPC_MESSAGE_ROUTED4(GpuVideoDecoderHostMsg_AllocateVideoFrames,
                    int32,  /* Number of video frames to generate */
                    uint32, /* Width of the video frame */
                    uint32, /* Height of the video frame */
                    int32   /* Format of the video frame */)

// Release all video frames allocated for a hardware video decoder.
IPC_MESSAGE_ROUTED0(GpuVideoDecoderHostMsg_ReleaseAllVideoFrames)

// GpuVideoDecoder report output format change.
IPC_MESSAGE_ROUTED1(GpuVideoDecoderHostMsg_MediaFormatChange,
                    GpuVideoDecoderFormatChangeParam)

// GpuVideoDecoder report error.
IPC_MESSAGE_ROUTED1(GpuVideoDecoderHostMsg_ErrorNotification,
                    GpuVideoDecoderErrorInfoParam)

// --------------------------------------------------------------------------
// TranspotTexture messages
//
// These are messages sent from renderer process to the GPU proess.

// Destroy corresponding TransportTexture in the GPU process.
IPC_MESSAGE_ROUTED0(GpuTransportTextureMsg_Destroy)

// Notify that the textures have been created. IDs are in client context.
IPC_MESSAGE_ROUTED1(GpuTransportTextureMsg_TexturesCreated,
                    std::vector<int> /* texture IDs */)

// --------------------------------------------------------------------------
//
// These are messages sent from GPU process to the renderer process.

// TransportTexture is created in the GPU process with the ID.
IPC_MESSAGE_ROUTED1(GpuTransportTextureHostMsg_TransportTextureCreated,
                    int32 /* id */)

// Allocate video frames for output of the hardware video decoder.
IPC_MESSAGE_ROUTED4(GpuTransportTextureHostMsg_CreateTextures,
                    int32,  /* Number of texturess to generate */
                    uint32, /* Width of the texture */
                    uint32, /* Height of the texture */
                    int32   /* Format of the texture */)

// Instruct the renderer to release all generated textures.
IPC_MESSAGE_ROUTED0(GpuTransportTextureHostMsg_ReleaseTextures)

// Notify a texture has been updated. The texture id is in the client context.
IPC_MESSAGE_ROUTED1(GpuTransportTextureHostMsg_TextureUpdated,
                    int /* texture_id */)

//------------------------------------------------------------------------------
// Accelerated Video Decoder Messages
// These messages are sent from Renderer process to GPU process.
// Message to query configuration information from the GPU process.
IPC_SYNC_MESSAGE_CONTROL1_1(AcceleratedVideoDecoderMsg_GetConfigs,
                            std::vector<uint32>, /* Proto config */
                            std::vector<uint32>) /* Matching configs */

// Message to create the accelerated video decoder.
IPC_SYNC_MESSAGE_CONTROL1_1(AcceleratedVideoDecoderMsg_Create,
                            std::vector<uint32>, /* Config */
                            int32) /* Decoder ID, 0 equals failure */

// Send input buffer for decoding.
IPC_MESSAGE_ROUTED4(AcceleratedVideoDecoderMsg_Decode,
                    base::SharedMemoryHandle, /* input_buffer_handle */
                    int32, /* offset */
                    int32, /* size */
                    int32) /* flags */

// Sent from Renderer process to the GPU process to give the texture IDs for
// generated GL textures.
IPC_MESSAGE_ROUTED2(AcceleratedVideoDecoderMsg_AssignPictureBuffer,
                    int32, /* Picture buffer ID */
                    std::vector<uint32>) /* TextureIDs for pictures */

IPC_MESSAGE_ROUTED1(AcceleratedVideoDecoderMsg_ReusePictureBuffer,
                    int32) /* Picture buffer ID */

// Send flush request to the decoder.
IPC_MESSAGE_ROUTED1(AcceleratedVideoDecoderMsg_Flush,
                    int32) /* 0 for normal flush, 1 for abort flush */

// Destroy and release decoder asynchronously.
IPC_SYNC_MESSAGE_CONTROL0_0(AcceleratedVideoDecoderMsg_Destroy)

//------------------------------------------------------------------------------
// Accelerated Video Decoder Host Messages
// These messages are sent from GPU process to Renderer process.
// Accelerated video decoder has consumed input buffer from transfer buffer.
IPC_MESSAGE_ROUTED0(AcceleratedVideoDecoderHostMsg_BitstreamBufferProcessed)

// Allocate video frames for output of the hardware video decoder.
IPC_MESSAGE_ROUTED4(AcceleratedVideoDecoderHostMsg_ProvidePictureBuffers,
                    int32,  /* Number of video frames to generate */
                    uint32, /* Width of the video frame */
                    uint32, /* Height of the video frame */
                    std::vector<uint32>) /* Vector containing the dictionary
                                            for buffer config */

// Decoder reports that a picture is ready and buffer does not need to be passed
// back to the decoder.
IPC_MESSAGE_ROUTED1(AcceleratedVideoDecoderHostMsg_DismissPictureBuffer,
                    int32) /* Picture buffer ID */

// Decoder reports that a picture is ready.
IPC_MESSAGE_ROUTED1(AcceleratedVideoDecoderHostMsg_PictureReady,
                    int32) /* Picture buffer ID */

// Confirm decoder has been flushed.
IPC_MESSAGE_ROUTED0(AcceleratedVideoDecoderHostMsg_FlushDone)

// Decoder has faced end of stream marker in the stream.
IPC_MESSAGE_ROUTED0(AcceleratedVideoDecoderHostMsg_EndOfStream)

// Video decoder has encountered an error.
IPC_MESSAGE_ROUTED1(AcceleratedVideoDecoderHostMsg_ErrorNotification,
                    uint32) /* Error ID */
