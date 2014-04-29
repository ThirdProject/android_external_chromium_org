// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sync/internal_api/public/sessions/status_counters.h"

#include "base/json/json_string_value_serializer.h"
#include "base/values.h"

namespace syncer {

StatusCounters::StatusCounters()
  : num_total_entries(0) {}

StatusCounters::~StatusCounters() {}

scoped_ptr<base::DictionaryValue> StatusCounters::ToValue() const {
  scoped_ptr<base::DictionaryValue> value(new base::DictionaryValue());
  value->SetInteger("numTotalEntries", num_total_entries);
  return value.Pass();
}

std::string StatusCounters::ToString() const {
  std::string result;
  scoped_ptr<base::DictionaryValue> value = ToValue();
  JSONStringValueSerializer serializer(&result);
  serializer.Serialize(*value);
  return result;
}

}  // namespace syncer
