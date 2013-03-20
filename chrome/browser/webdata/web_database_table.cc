// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/webdata/web_database_table.h"

WebDatabaseTable::WebDatabaseTable() : db_(NULL), meta_table_(NULL) {
}

WebDatabaseTable::~WebDatabaseTable() {
}

bool WebDatabaseTable::Init(sql::Connection* db, sql::MetaTable* meta_table) {
  db_ = db;
  meta_table_ = meta_table;
  return true;
}
