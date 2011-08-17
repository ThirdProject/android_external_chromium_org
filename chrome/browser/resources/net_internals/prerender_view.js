// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * This view displays information related to Prerendering.
 */
var PrerenderView = (function() {
  'use strict';

  // IDs for special HTML elements in prerender_view.html
  var MAIN_BOX_ID = 'prerender-view-tab-content';
  var ENABLED_SPAN_ID = 'prerender-view-enabled-span';
  var HISTORY_DIV_ID = 'prerender-view-history-div';
  var ACTIVE_DIV_ID = 'prerender-view-active-div';

  // We inherit from DivView.
  var superClass = DivView;

  /**
   * @constructor
   */
  function PrerenderView() {
    assertFirstConstructorCall(PrerenderView);

    // Call superclass's constructor.
    superClass.call(this, MAIN_BOX_ID);

    g_browser.addPrerenderInfoObserver(this);
    this.prerenderEnabledSpan_ = $(ENABLED_SPAN_ID);
    this.prerenderHistoryDiv_ = $(HISTORY_DIV_ID);
    this.prerenderActiveDiv_ = $(ACTIVE_DIV_ID);
  }

  // ID for special HTML element in category_tabs.html
  PrerenderView.TAB_HANDLE_ID = 'tab-handle-prerender';

  cr.addSingletonGetter(PrerenderView);

  PrerenderView.prototype = {
    // Inherit the superclass's methods.
    __proto__: superClass.prototype,

    onLoadLogFinish: function(data) {
      return this.onPrerenderInfoChanged(data.prerenderInfo);
    },

    onPrerenderInfoChanged: function(prerenderInfo) {
      this.prerenderEnabledSpan_.textContent = '';
      this.prerenderHistoryDiv_.innerHTML = '';
      this.prerenderActiveDiv_.innerHTML = '';

      if (!isValidPrerenderInfo(prerenderInfo))
        return false;

      this.prerenderEnabledSpan_.textContent = prerenderInfo.enabled.toString();

      var tabPrinter = createHistoryTablePrinter(prerenderInfo.history);
      tabPrinter.toHTML(this.prerenderHistoryDiv_, 'styledTable');

      var tabPrinter = createActiveTablePrinter(prerenderInfo.active);
      tabPrinter.toHTML(this.prerenderActiveDiv_, 'styledTable');

      return true;
    }
  };

  function isValidPrerenderInfo(prerenderInfo) {
    if (prerenderInfo == null) {
      return false;
    }
    if (!('history' in prerenderInfo) ||
        !('active' in prerenderInfo) ||
        !('enabled' in prerenderInfo)) {
      return false;
    }
    return true;
  }

  function createHistoryTablePrinter(prerenderHistory) {
    var tablePrinter = new TablePrinter();
    tablePrinter.addHeaderCell('Origin');
    tablePrinter.addHeaderCell('URL');
    tablePrinter.addHeaderCell('Final Status');
    tablePrinter.addHeaderCell('Time');

    for (var i = 0; i < prerenderHistory.length; i++) {
      var historyEntry = prerenderHistory[i];
      tablePrinter.addRow();
      tablePrinter.addCell(historyEntry.origin);
      tablePrinter.addCell(historyEntry.url);
      tablePrinter.addCell(historyEntry.final_status);

      var date = new Date(parseInt(historyEntry.end_time));
      tablePrinter.addCell(date.toLocaleString());
    }
    return tablePrinter;
  }

  function createActiveTablePrinter(prerenderActive) {
    var tablePrinter = new TablePrinter();
    tablePrinter.addHeaderCell('URL');
    tablePrinter.addHeaderCell('Duration');

    for (var i = 0; i < prerenderActive.length; i++) {
      var activeEntry = prerenderActive[i];
      tablePrinter.addRow();
      tablePrinter.addCell(activeEntry.url);
      tablePrinter.addCell(activeEntry.duration);
    }
    return tablePrinter;
  }

  return PrerenderView;
})();
