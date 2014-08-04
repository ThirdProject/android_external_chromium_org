// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/common/permissions/manifest_permission.h"

#include "base/json/json_writer.h"
#include "extensions/common/manifest_handler.h"
#include "ipc/ipc_message.h"
#include "ipc/ipc_message_utils.h"

namespace extensions {

ManifestPermission::ManifestPermission() {}

ManifestPermission::~ManifestPermission() { }

ManifestPermission* ManifestPermission::Clone() const {
  return Union(this);
}

bool ManifestPermission::Contains(const ManifestPermission* rhs) const {
  return scoped_ptr<ManifestPermission>(Intersect(rhs))->Equal(rhs);
}

bool ManifestPermission::Equal(const ManifestPermission* rhs) const {
  return ToValue()->Equals(rhs->ToValue().get());
}

void ManifestPermission::Write(IPC::Message* m) const {
  base::ListValue singleton;
  base::Value* value = ToValue().release();
  singleton.Append(value);
  IPC::WriteParam(m, singleton);
}

bool ManifestPermission::Read(const IPC::Message* m, PickleIterator* iter) {
  base::ListValue singleton;
  if (!IPC::ReadParam(m, iter, &singleton))
    return false;
  if (singleton.GetSize() != 1)
    return false;
  base::Value* value = NULL;
  if (!singleton.Get(0, &value))
    return false;
  return FromValue(value);
}

void ManifestPermission::Log(std::string* log) const {
  base::JSONWriter::WriteWithOptions(
      ToValue().get(), base::JSONWriter::OPTIONS_PRETTY_PRINT, log);
}

}  // namespace extensions
