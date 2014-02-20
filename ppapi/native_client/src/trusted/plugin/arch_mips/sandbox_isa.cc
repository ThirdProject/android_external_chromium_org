// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/native_client/src/trusted/plugin/nexe_arch.h"

namespace {
// The list of supported ISA strings for MIPS.  See issue:
//   http://code.google.com/p/nativeclient/issues/detail?id=1040 for more
// information.  Note that these string are to be case-insensitive compared.
const char* const kNexeArchMIPS = "mips32";
}  // namespace

namespace plugin {
const char* GetSandboxISA() {
  return kNexeArchMIPS;
}
}  // namespace plugin
