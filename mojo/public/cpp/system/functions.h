// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_SYSTEM_FUNCTIONS_H_
#define MOJO_PUBLIC_CPP_SYSTEM_FUNCTIONS_H_

#include "mojo/public/c/system/functions.h"

namespace mojo {

// Standalone functions --------------------------------------------------------

inline MojoTimeTicks GetTimeTicksNow() {
  return MojoGetTimeTicksNow();
}

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_SYSTEM_FUNCTIONS_H_
