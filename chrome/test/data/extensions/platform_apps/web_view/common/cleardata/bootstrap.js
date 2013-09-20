// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

config.IS_CHROME_TEST = true;
// Guest served from TestServer.
config.IS_JS_ONLY_GUEST = false;
config.TEST_DIR = 'cleardata';

var clearDataTests = {};

// step1. Ask guest to load some cookie.
// step2. Guest responds saying it has added cookies.
// embedder clears cookie data of the guest via clearData API.
// step3. Ask guest for cookies that were set in step1.
// step4. Guest responds with cookie values, embedder verifies they are unset.

var run = function() {
  var container = document.createElement('div');
  container.id = 'webview-tag-container';
  document.body.appendChild(container);

  chrome.test.getConfig(function(chromeConfig) {
    window.console.log('getConfig: ' + chromeConfig);
    utils.setUp(chromeConfig, config);
    embedder.loadGuest(function() {
      chrome.test.runTests([
        clearDataTests.testCookies
      ]);
    }, function(data) {
      var handled = true;
      switch (data[0]) {
        case 'step2.cookies-added':
          window.console.log('embedder, on message: ' + data[0]);
          var onDataCleared = function() {
            window.console.log('embedder.onDataCleared');
            embedder.webview.contentWindow.postMessage(
                JSON.stringify(['step3.get-cookies', 'foo', 'bar']), '*');
          };
          embedder.webview.clearData(
              {'since': 1}, {'cookies': true}, onDataCleared);
          break;
        case 'step4.got-cookies':
          window.console.log('embedder, on message: ' + data[0]);
          var cookies = data[1];
          chrome.test.assertEq([null, null], cookies);
          chrome.test.succeed();
          break;
        default:
          handled = false;
          break;
      }
      return handled;
    });
  });
};

// Tests.
clearDataTests.testCookies = function testCookies() {
  window.console.log('clearDataTests.testCookies');
  embedder.webview.contentWindow.postMessage(
      JSON.stringify(['step1.add-cookies']), '*');
};

// Run test(s).
run();
