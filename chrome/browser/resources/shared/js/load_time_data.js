// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview This file defines a singleton which provides access to all data
 * that is available as soon as the page's resources are loaded (before DOM
 * content has finished loading). This data includes both localized strings and
 * any data that is important to have ready from a very early stage (e.g. things
 * that must be displayed right away).
 */

var loadTimeData;

(function() {
  'use strict';

  function LoadTimeData() {
  }

  LoadTimeData.prototype = {
    /**
     * Sets the backing object.
     * @param {Object} value The de-serialized page data.
     */
    set data(value) {
      assert(!this.data_, 'Re-setting data.');
      this.data_ = value;
    },

    /**
     * Fetches a value, asserting that it exists.
     * @param {string} id The key that identifies the desired value.
     * @return {*} The corresponding value.
     */
    getValue: function(id) {
      assert(this.data_, 'No data. Did you remember to include strings.js?');
      var value = this.data_[id];
      assert(typeof value != 'undefined', 'Could not find value for ' + id);
      return value;
    },

    /**
     * As above, but also makes sure that the value is a string.
     * @param {string} id The key that identifies the desired string.
     * @return {string} The corresponding string value.
     */
    getString: function(id) {
      var value = this.getValue(id);
      assert(typeof value == 'string', '[' + value + '] (' + id +
                                       ') is not a string');
      return value;
    },
  };

  /**
   * Asserts, displays error message if assert fails.
   * @param {*} condition The condition to check for truthiness.
   * @param {string} message The message to display if the check fails.
   */
  function assert(condition, message) {
    if (!condition) {
      console.error(message);
    }
  }

  assert(!loadTimeData, 'should only include this file once');
  loadTimeData = new LoadTimeData;
})();
