// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview The way these tests work is as follows:
 * C++ in net_internals_ui_browsertest.cc does any necessary setup, and then
 * calls the entry point for a test with RunJavascriptTest.  The called
 * function can then use the assert/expect functions defined in test_api.js.
 * All callbacks from the browser are wrapped in such a way that they can
 * also use the assert/expect functions.
 *
 * A test ends when an assert/expect test fails, an exception is thrown, or
 * |netInternalsTest.testDone| is called.  At that point, or soon afterwards,
 * the title is updated to 'Test Failed' if an assert/expect test fails, or
 * there was an exception.  Otherwise, it's set to 'Test Passed'.  The
 * behavior when an assert/expect test fails or an assertion is thrown only
 * after |netInternalsTest.testDone| is called is undefined.
 */

// Start of namespace.
var netInternalsTest = (function() {
  /**
   * A shorter poll interval is used for tests, since a few tests wait for
   * polled values to change.
   * @type {number}
   * @const
   */
  var TESTING_POLL_INTERVAL_MS = 50;

  /**
   * Map of tab handle names to location hashes.
   * @type {object.<string, string>}
   */
  var hashToTabHandleIdMap = {
    capture: CaptureView.TAB_HANDLE_ID,
    export: ExportView.TAB_HANDLE_ID,
    import: ImportView.TAB_HANDLE_ID,
    proxy: ProxyView.TAB_HANDLE_ID,
    events: EventsView.TAB_HANDLE_ID,
    dns: DnsView.TAB_HANDLE_ID,
    sockets: SocketsView.TAB_HANDLE_ID,
    spdy: SpdyView.TAB_HANDLE_ID,
    httpCache: HttpCacheView.TAB_HANDLE_ID,
    httpThrottling: HttpThrottlingView.TAB_HANDLE_ID,
    serviceProviders: ServiceProvidersView.TAB_HANDLE_ID,
    tests: TestView.TAB_HANDLE_ID,
    hsts: HSTSView.TAB_HANDLE_ID,
    logs: LogsView.TAB_HANDLE_ID,
    prerender: PrerenderView.TAB_HANDLE_ID
  };

  /**
   * Indicates if the test is complete.
   * @type {boolean}
   */
  var done = false;

  /**
   * Updates the title of the page to report success or failure.  Must be
   * called at most once for each test.
   * @param {boolean} success Description of success param.
   */
  function updateTitle(success) {
    if (success) {
      document.title = 'Test Passed';
    } else {
      document.title = 'Test Failed';
    }
    done = true;
  }

  /**
   * Called to indicate a test is complete.
   */
  function testDone() {
    done = true;
  }

  /**
   * Creates a test function that can use the expect and assert functions
   * in test_api.js.  On failure, will set title to 'Test Failed', and when
   * a test is done and there was no failure, will set title to 'Test Passed'.
   * Calling expect/assert functions after done has been called has undefined
   * behavior.  Returned test functions can safely call each other directly.
   *
   * The resulting function has no return value.
   * @param {string} testName The name of the function, reported on error.
   * @param {Function} testFunction The function to run.
   * @return {function():void} Function that passes its parameters to
   *     testFunction, and passes the test result, if any, to the browser
   *     process by setting the window title.
   */
  function createTestFunction(testName, testFunction) {
    return function() {
      // Convert arguments to an array, as their map method may be called on
      // failure by runTestFunction.
      var testArguments = Array.prototype.slice.call(arguments, 0);

      // If the test is already complete, do nothing.
      if (done)
        return;

      var result = runTestFunction(testName, testFunction, testArguments);

      // If the first value is false, the test failed.
      if (!result[0]) {
        // Print any error messages.
        console.log(result[1]);
        // Update title to indicate failure.
        updateTitle(false);
      } else if (done)  {
        // If the first result is true, and |done| is also true, the test
        // passed.  Update title to indicate success.
        updateTitle(true);
      }
    };
  }

  /**
   * Dictionary of tests.
   * @type {Object.<string, Function>}
   */
  var tests = {};

  /**
   * Used to declare a test function called by the NetInternals browser test.
   * Takes in a name and a function, and adds it to the list of tests.
   * @param {string} testName The of the test.
   * @param {Function} testFunction The test function.
   */
  function test(testName, testFunction) {
    tests[testName] = testFunction;
  }

  /**
   * Called by the browser to start a test.  If constants haven't been
   * received from the browser yet, waits until they have been.
   * Experimentally, this never seems to happen, but may theoretically be
   * possible.
   * @param {string} testName The of the test to run.
   * @param {Function} testArguments The test arguments.
   */
  function runTest(testName, testArguments) {
    // If we've already received the constants, start the tests.
    if (typeof(LogEventType) != 'undefined') {
      startNetInternalsTest(testName, testArguments);
      return;
    }

    // Otherwise, wait until we do.
    console.log('Received constants late.');

    /**
     * Observer that starts the tests once we've received the constants.
     */
    function ConstantsObserver() {
      this.testStarted_ = false;
    }

    ConstantsObserver.prototype.onConstantsReceived = function() {
      if (!this.testStarted_) {
        this.testStarted_ = true;
        startNetInternalsTest(testFunction, testArguments);
      }
    };

    g_browser.addConstantsObserver(new ConstantsObserver());
  }

  /**
   * Starts running the test.  A test is run until an assert/expect statement
   * fails or testDone is called.  Those functions can only be called in the
   * test function body, or in response to a message dispatched by
   * |g_browser.receive|.
   * @param {string} testName The of the test to run.
   * @param {Function} testArguments The test arguments.
   */
  function startNetInternalsTest(testName, testArguments) {
    // Wrap g_browser.receive around a test function so that assert and expect
    // functions can be called from observers.
    g_browser.receive = createTestFunction('g_browser.receive', function() {
      BrowserBridge.prototype.receive.apply(g_browser, arguments);
    });

    g_browser.setPollInterval(TESTING_POLL_INTERVAL_MS);
    createTestFunction(testName, tests[testName]).apply(null, testArguments);
  }

  /**
   * Finds the first styled table that's a child of |parentId|, and returns the
   * number of rows it has.  Returns -1 if there's no such table.
   * @param {string} parentId HTML element id containing a styled table.
   * @return {number} Number of rows the style table's body has.
   */
  function getStyledTableNumRows(parentId) {
    // The tbody element of the first styled table in |parentId|.
    var tbody = document.querySelector('#' + parentId + ' .styledTable tbody');
    if (!tbody)
      return -1;
    return tbody.children.length;
  }

  /**
   * Finds the first styled table that's a child of the element with the given
   * id, and checks if it has exactly |expectedRows| rows, not including the
   * header row.
   * @param {string} parentId HTML element id containing a styled table.
   * @param {number} expectedRows Expected number of rows in the table.
   */
  function checkStyledTableRows(parentId, expectedRows) {
    expectEquals(expectedRows, getStyledTableNumRows(parentId),
                 'Incorrect number of rows in ' + parentId);
  }

  /**
   * Returns the TabEntry with the given id.  Asserts if the tab can't be found.
   * @param {string}: tabId Id of the TabEntry to get.
   * @return {TabEntry} The specified TabEntry.
   */
  function getTab(tabId) {
    var categoryTabSwitcher = MainView.getInstance().categoryTabSwitcher();
    var tab = categoryTabSwitcher.findTabById(tabId);
    assertNotEquals(tab, undefined, tabId + ' does not exist.');
    return tab;
  }

  /**
   * Returns true if the specified tab's handle is visible, false otherwise.
   * Asserts if the handle can't be found.
   * @param {string}: tabId Id of the tab to check.
   * @return {bool} Whether or not the tab's handle is visible.
   */
  function tabHandleIsVisible(tabId) {
    var tabHandleNode = getTab(tabId).getTabHandleNode();
    return tabHandleNode.style.display != 'none';
  }

  /**
   * Returns the tab id of a tab, given its associated URL hash value.  Asserts
   *    if |hash| has no associated tab.
   * @param {string}: hash Hash associated with the tab to return the id of.
   * @return {string} String identifier of the tab with the given hash.
   */
  function getTabId(hash) {
    assertEquals(typeof hashToTabHandleIdMap[hash], 'string',
                 'Invalid tab anchor: ' + hash);
    var tabId = hashToTabHandleIdMap[hash];
    assertEquals('object', typeof getTab(tabId), 'Invalid tab: ' + tabId);
    return tabId;
  }

  /**
   * Switches to the specified tab.
   * @param {string}: hash Hash associated with the tab to switch to.
   */
  function switchToView(hash) {
    var tabId = getTabId(hash);

    // Make sure the tab handle is visible, as we only simulate normal usage.
    expectTrue(tabHandleIsVisible(tabId),
               tabId + ' does not have a visible tab handle.');
    var tabHandleNode = getTab(tabId).getTabHandleNode();

    // Simulate a left click.
    var mouseEvent = document.createEvent('MouseEvents');
    mouseEvent.initMouseEvent('click', true, true, window,
                              1, 0, 0, 0, 0,
                              false, false, false, false, 0, null);
    $(tabId).dispatchEvent(mouseEvent);

    // Make sure the hash changed.
    assertEquals('#' + hash, document.location.hash);

    // Run the onhashchange function, so we can test the resulting state.
    // Otherwise, won't trigger until after we return.
    window.onhashchange();

    // Make sure only the specified tab is visible.
    var categoryTabSwitcher = MainView.getInstance().categoryTabSwitcher();
    var tabIds = categoryTabSwitcher.getAllTabIds();
    for (var i = 0; i < tabIds.length; ++i) {
      expectEquals(tabIds[i] == tabId,
                   getTab(tabIds[i]).contentView.isVisible(),
                   tabIds[i] + ': Unexpected visibility state.');
    }
  }

  /**
   * Checks the visibility of all tab handles against expected values.
   * @param {object.<string, bool>}: tabVisibilityState Object with a an entry
   *     for each tab's hash, and a bool indicating if it should be visible or
   *     not.
   * @param {bool+}: tourTabs True if tabs expected to be visible should should
   *     each be navigated to as well.
   */
  function checkTabHandleVisibility(tabVisibilityState, tourTabs) {
    // Check visibility state of all tabs.
    var tabCount = 0;
    for (var hash in tabVisibilityState) {
      var tabId = getTabId(hash);
      assertEquals('object', typeof getTab(tabId), 'Invalid tab: ' + tabId);
      expectEquals(tabVisibilityState[hash], tabHandleIsVisible(tabId),
                   tabId + ' visibility state is unexpected.');
      if (tourTabs && tabVisibilityState[hash])
        switchToView(hash);
      tabCount++;
    }

    // Check that every tab was listed.
    var categoryTabSwitcher = MainView.getInstance().categoryTabSwitcher();
    var tabIds = categoryTabSwitcher.getAllTabIds();
    expectEquals(tabCount, tabIds.length);
  }

  // Exported functions.
  return {
    test: test,
    runTest: runTest,
    testDone:testDone,
    checkStyledTableRows: checkStyledTableRows,
    switchToView: switchToView,
    checkTabHandleVisibility: checkTabHandleVisibility
  };
})();

netInternalsTest.test('NetInternalsDone', function() {
  netInternalsTest.testDone();
});

netInternalsTest.test('NetInternalsExpectFail', function() {
  expectNotReached();
});

netInternalsTest.test('NetInternalsAssertFail', function() {
  assertNotReached();
});

netInternalsTest.test('NetInternalsObserverDone', function() {
  /**
   * A HostResolverInfo observer that calls testDone() in response to the
   * first seen event.
   */
  function HostResolverInfoObserver() {
  }

  HostResolverInfoObserver.prototype.onHostResolverInfoChanged = function() {
    netInternalsTest.testDone();
  };

  // Create the observer and add it to |g_browser|.
  g_browser.addHostResolverInfoObserver(new HostResolverInfoObserver());

  // Needed to trigger an update.
  netInternalsTest.switchToView('dns');
});

netInternalsTest.test('NetInternalsObserverExpectFail', function() {
  /**
   * A HostResolverInfo observer that triggers an exception in response to the
   * first seen event.
   */
  function HostResolverInfoObserver() {
  }

  HostResolverInfoObserver.prototype.onHostResolverInfoChanged = function() {
    expectNotReached();
    netInternalsTest.testDone();
  };

  // Create the observer and add it to |g_browser|.
  g_browser.addHostResolverInfoObserver(new HostResolverInfoObserver());

  // Needed to trigger an update.
  netInternalsTest.switchToView('dns');
});

netInternalsTest.test('NetInternalsObserverAssertFail', function() {
  /**
   * A HostResolverInfo observer that triggers an assertion in response to the
   * first seen event.
   */
  function HostResolverInfoObserver() {
  }

  HostResolverInfoObserver.prototype.onHostResolverInfoChanged = function() {
    assertNotReached();
  };

  // Create the observer and add it to |g_browser|.
  g_browser.addHostResolverInfoObserver(new HostResolverInfoObserver());

  // Needed to trigger an update.
  netInternalsTest.switchToView('dns');
});
