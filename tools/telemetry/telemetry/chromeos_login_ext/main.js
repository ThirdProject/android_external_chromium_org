// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var PARENT_PAGE = 'chrome://oobe/';

var msg = {
  'method': 'loginUILoaded'
};
window.parent.postMessage(msg, PARENT_PAGE);

var msg = {
  'method': 'completeLogin',
  'email': 'test@test.test',
  'password': ''
};
window.parent.postMessage(msg, PARENT_PAGE);
