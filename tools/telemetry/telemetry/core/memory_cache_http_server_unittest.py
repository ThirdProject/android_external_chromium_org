# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

from telemetry.core import util
from telemetry.unittest import tab_test_case


class MemoryCacheHTTPServerTest(tab_test_case.TabTestCase):
  def setUp(self):
    super(MemoryCacheHTTPServerTest, self).setUp()
    self.test_file = os.path.join(util.GetUnittestDataDir(), 'bear.webm')
    self.test_file_size = os.stat(self.test_file).st_size

  def testBasicHostingAndRangeRequests(self):
    self.Navigate('blank.html')
    x = self._tab.EvaluateJavaScript('document.body.innerHTML')
    x = x.strip()

    # Test basic html hosting.
    self.assertEquals(x, 'Hello world')

    file_size = self.test_file_size
    last_byte = file_size - 1
    # Test byte range request: no end byte.
    self.CheckContentHeaders('0-', '0-%d' % last_byte, file_size)

    # Test byte range request: greater than zero start byte.
    self.CheckContentHeaders('100-', '100-%d' % last_byte,
                             file_size - 100)

    # Test byte range request: explicit byte range.
    self.CheckContentHeaders('2-500', '2-500', '499')

    # Test byte range request: no start byte.
    self.CheckContentHeaders('-228',
                             '%d-%d' % (file_size - 228, last_byte),
                             '228')

    # Test byte range request: end byte less than start byte.
    self.CheckContentHeaders('100-5', '100-%d' % last_byte,
                             file_size - 100)

  def CheckContentHeaders(self, content_range_request, content_range_response,
                          content_length_response):
    self._tab.ExecuteJavaScript("""
        var loaded = false;
        var xmlhttp = new XMLHttpRequest();
        xmlhttp.onload = function(e) {
          loaded = true;
        };
        // Avoid cached content by appending unique URL param.
        xmlhttp.open('GET', "%s?t=" + Date.now(), true);
        xmlhttp.setRequestHeader('Range', 'bytes=%s');
        xmlhttp.send();
    """ % (self._browser.http_server.UrlOf(self.test_file),
           content_range_request))
    self._tab.WaitForJavaScriptExpression('loaded', 5)
    content_range = self._tab.EvaluateJavaScript(
        'xmlhttp.getResponseHeader("Content-Range");')
    content_range_response = 'bytes %s/%d' % (
        content_range_response, self.test_file_size)
    self.assertEquals(content_range, content_range_response)
    content_length = self._tab.EvaluateJavaScript(
        'xmlhttp.getResponseHeader("Content-Length");')
    self.assertEquals(content_length, str(content_length_response))
