// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/glue/invalidation_adapter.h"

#include <string>

namespace browser_sync {

InvalidationAdapter::InvalidationAdapter(
    const syncer::Invalidation& invalidation)
    : invalidation_(invalidation) {
}

InvalidationAdapter::~InvalidationAdapter() {
}

bool InvalidationAdapter::IsUnknownVersion() const {
  return invalidation_.is_unknown_version();
}

const std::string& InvalidationAdapter::GetPayload() const {
  return invalidation_.payload();
}

int64 InvalidationAdapter::GetVersion() const {
  return invalidation_.version();
}

void InvalidationAdapter::Acknowledge() {
  invalidation_.Acknowledge();
}

void InvalidationAdapter::Drop() {
  invalidation_.Drop();
}

}  // namespace browser_sync
