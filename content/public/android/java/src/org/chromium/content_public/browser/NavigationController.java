// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content_public.browser;

/**
 * The NavigationController Java wrapper to allow communicating with the native
 * NavigationController object.
 */
public interface NavigationController {
    /**
     * @return Whether back navigation is possible from the "current entry".
     */
    boolean canGoBack();

    /**
     * @return Whether forward navigation is possible from the "current entry".
     */
    boolean canGoForward();

    /**
     * @param offset The offset into the navigation history.
     * @return Whether we can move in history by given offset
     */
    boolean canGoToOffset(int offset);

    /**
     * Navigates to the specified offset from the "current entry". Does nothing if the offset is
     * out of bounds.
     * @param offset The offset into the navigation history.
     */
    void goToOffset(int offset);

    /**
     * Navigates to the specified index in the navigation entry for this page.
     * @param index The navigation index to navigate to.
     */
    void goToNavigationIndex(int index);

    /**
     * Goes to the navigation entry before the current one.
     */
    void goBack();

    /**
     * Goes to the navigation entry following the current one.
     */
    void goForward();

    /**
     * Loads the current navigation if there is a pending lazy load (after tab restore).
     */
    public void loadIfNecessary();

    /**
     * Requests the current navigation to be loaded upon the next call to loadIfNecessary().
     */
    public void requestRestoreLoad();

    /**
     * Reload the current page.
     */
    public void reload(boolean checkForRepost);

    /**
     * Reload the current page, ignoring the contents of the cache.
     */
    public void reloadIgnoringCache(boolean checkForRepost);

    /**
     * Cancel the pending reload.
     */
    public void cancelPendingReload();

    /**
     * Continue the pending reload.
     */
    public void continuePendingReload();
}
