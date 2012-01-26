// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_CPP_DEV_VAR_ARRAY_BUFFER_DEV_H_
#define PPAPI_CPP_DEV_VAR_ARRAY_BUFFER_DEV_H_

#include "ppapi/cpp/var.h"

/// @file
/// This file defines the API for interacting with an ArrayBuffer.

namespace pp {

/// VarArrayBuffer_Dev provides a way to interact with JavaScript ArrayBuffers,
/// which represent a contiguous sequence of bytes. Note that
/// VarArrayBuffer_Devs are not part of the embedding page's DOM, and can only
/// be shared with JavaScript via pp::Instance's PostMessage and HandleMessage
/// functions.
class VarArrayBuffer_Dev : public Var {
 public:
  /// Contruct a VarArrayBuffer_Dev given a var for which is_array_buffer() is
  /// true. This will refer to the same ArrayBuffer as var, but allows you to
  /// access methods specific to VarArrayBuffer_Dev.
  ///
  /// @param[in] var An ArrayBuffer Var.
  explicit VarArrayBuffer_Dev(const Var& var);

  /// Copy constructor.
  VarArrayBuffer_Dev(const VarArrayBuffer_Dev& buffer) : Var(buffer) {}

  virtual ~VarArrayBuffer_Dev() {}

  /// This function assigns one VarArrayBuffer to another VarArrayBuffer.
  ///
  /// @param[in] other The VarArrayBuffer to be assigned.
  ///
  /// @return The resulting VarArrayBuffer.
  VarArrayBuffer_Dev& operator=(const VarArrayBuffer_Dev& other);

  /// This function assigns one VarArrayBuffer to another VarArrayBuffer. Var's
  /// assignment operator is overloaded here so that we can check for assigning
  /// a non-ArrayBuffer var to a VarArrayBuffer_Dev.
  ///
  /// @param[in] other The VarArrayBuffer to be assigned.
  ///
  /// @return The resulting VarArrayBuffer (as a Var&).
  virtual Var& operator=(const Var& other);

  /// Construct a new VarArrayBuffer_Dev which is size_in_bytes bytes long and
  /// initialized to zero.
  ///
  /// @param[in] size_in_bytes The size of the constructed ArrayBuffer in bytes.
  VarArrayBuffer_Dev(uint32_t size_in_bytes);

  /// Return the length of the VarArrayBuffer_Dev in bytes.
  ///
  /// @return The length of the VarArrayBuffer_Dev in bytes.
  uint32_t ByteLength() const;

  /// Maps the ArrayBuffer in to the module's address space and returns a
  /// pointer to the internal buffer for this ArrayBuffer.
  ///
  /// Note that calling Map() can be a relatively expensive operation. Use care
  /// when calling it in performance-critical code. For example, you should call
  /// it only once when looping over an ArrayBuffer:
  ///
  /// <code>
  ///   char* data = static_cast<char*>(array_buffer_var.Map());
  ///   uint32_t byte_length = array_buffer_var.ByteLength();
  ///   for (uint32_t i = 0; i < byte_length; ++i)
  ///     data[i] = 'A';
  /// </code>
  ///
  /// @return A pointer to the internal buffer for this ArrayBuffer.
  void* Map();

  /// Unmaps this ArrayBuffer var from the module address space. Use this if
  /// you want to save memory but might want to Map the buffer again later.
  void Unmap();
};

}  // namespace pp

#endif  // PPAPI_CPP_DEV_VAR_ARRAY_BUFFER_DEV_H_
