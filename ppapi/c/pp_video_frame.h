/* Copyright (c) 2013 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* From pp_video_frame.idl modified Tue Apr  2 10:07:49 2013. */

#ifndef PPAPI_C_PP_VIDEO_FRAME_H_
#define PPAPI_C_PP_VIDEO_FRAME_H_

#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_stdint.h"
#include "ppapi/c/pp_time.h"

/**
 * @file
 * This file defines the struct used to hold a video frame.
 */


/**
 * @addtogroup Structs
 * @{
 */
/**
 * The <code>PP_Video_Frame</code> struct represents a video frame.
 */
struct PP_VideoFrame {
  /**
   * A timestamp placing the frame in a video stream.
   */
  PP_TimeTicks timestamp;
  /**
   * An image data resource to hold the video frame.
   */
  PP_Resource image_data;
  /**
   * Ensure that this struct is 16-bytes wide by padding the end.  In some
   * compilers, PP_TimeTicks is 8-byte aligned, so those compilers align this
   * struct on 8-byte boundaries as well and pad it to 8 bytes even without this
   * padding attribute.  This padding makes its size consistent across
   * compilers.
   */
  int32_t padding;
};
PP_COMPILE_ASSERT_STRUCT_SIZE_IN_BYTES(PP_VideoFrame, 16);
/**
 * @}
 */

#endif  /* PPAPI_C_PP_VIDEO_FRAME_H_ */

