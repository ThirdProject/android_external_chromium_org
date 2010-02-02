// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file includes all the necessary GL headers and implements some useful
// utilities.

#ifndef GPU_COMMAND_BUFFER_SERVICE_GL_UTILS_H_
#define GPU_COMMAND_BUFFER_SERVICE_GL_UTILS_H_

#include <build/build_config.h>

#if defined(UNIT_TEST)
#include "gpu/command_buffer/service/gl_mock.h"
#else
#include <GL/glew.h>
#if defined(OS_WIN)
#include <GL/wglew.h>
#elif defined(OS_LINUX)
#include <GL/glxew.h>
#endif  // OS_WIN
#endif  // UNIT_TEST

#define GL_GLEXT_PROTOTYPES 1

// Define this for extra GL error debugging (slower).
// #define GL_ERROR_DEBUGGING
#ifdef GL_ERROR_DEBUGGING
#define CHECK_GL_ERROR() do {                                           \
    GLenum gl_error = glGetError();                                     \
    LOG_IF(ERROR, gl_error != GL_NO_ERROR) << "GL Error :" << gl_error; \
  } while (0)
#else  // GL_ERROR_DEBUGGING
#define CHECK_GL_ERROR() void(0)
#endif  // GL_ERROR_DEBUGGING

#endif  // GPU_COMMAND_BUFFER_SERVICE_GL_UTILS_H_
