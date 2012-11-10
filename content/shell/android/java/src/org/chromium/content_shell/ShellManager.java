// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content_shell;

import android.content.Context;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.widget.FrameLayout;

import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;
import org.chromium.content.browser.ContentViewRenderView;
import org.chromium.ui.gfx.NativeWindow;

/**
 * Container and generator of ShellViews.
 */
@JNINamespace("content")
public class ShellManager extends FrameLayout {

    private NativeWindow mWindow;
    private Shell mActiveShell;

    private String mStartupUrl = ContentShellActivity.DEFAULT_SHELL_URL;

    // The target for all content rendering.
    private ContentViewRenderView mContentViewRenderView;

    /**
     * Constructor for inflating via XML.
     */
    public ShellManager(Context context, AttributeSet attrs) {
        super(context, attrs);
        nativeInit(this);
        mContentViewRenderView = new ContentViewRenderView(context) {
            @Override
            protected void onReadyToRender() {
                mActiveShell.loadUrl(mStartupUrl);
            }
        };
    }

    /**
     * @param window The window used to generate all shells.
     */
    public void setWindow(NativeWindow window) {
        mWindow = window;
    }

    /**
     * @return The window used to generate all shells.
     */
    public NativeWindow getWindow() {
        return mWindow;
    }

    /**
     * Sets the startup URL for new shell windows.
     */
    public void setStartupUrl(String url) {
        mStartupUrl = url;
    }

    /**
     * @return The currently visible shell view or null if one is not showing.
     */
    protected Shell getActiveShell() {
        return mActiveShell;
    }

    /**
     * Creates a new shell pointing to the specified URL.
     * @param url The URL the shell should load upon creation.
     */
    public void launchShell(String url) {
        nativeLaunchShell(url);
    }

    @SuppressWarnings("unused")
    @CalledByNative
    private int getContentViewLayerRenderer() {
        return mContentViewRenderView.getNativeContentViewLayerRenderer();
    }

    @SuppressWarnings("unused")
    @CalledByNative
    private Object createShell() {
        LayoutInflater inflater =
                (LayoutInflater) getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        Shell shellView = (Shell) inflater.inflate(R.layout.shell_view, null);
        shellView.setWindow(mWindow);

        removeAllViews();
        if (mActiveShell != null) {
            if (mActiveShell.getContentView() != null) {
                mActiveShell.getContentView().onHide();
            }
            mActiveShell.setContentViewRenderView(null);
        }

        shellView.setContentViewRenderView(mContentViewRenderView);
        addView(shellView, new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT));
        mActiveShell = shellView;
        if (mActiveShell.getContentView() != null) mActiveShell.getContentView().onShow();

        return shellView;
    }

    private static native void nativeInit(Object shellManagerInstance);
    private static native void nativeLaunchShell(String url);
}
