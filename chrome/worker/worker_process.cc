// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/worker/worker_process.h"

#include "chrome/worker/worker_thread.h"

WorkerProcess::WorkerProcess()
  : ChildProcess(new WorkerThread()) {
}

WorkerProcess::~WorkerProcess() {
}
