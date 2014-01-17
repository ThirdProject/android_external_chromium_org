// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.test;

import android.test.suitebuilder.annotation.MediumTest;

import org.chromium.android_webview.AwContents;
import org.chromium.android_webview.AwSettings;
import org.chromium.base.test.util.Feature;
import org.chromium.content.browser.test.util.CallbackHelper;
import org.chromium.ui.gfx.DeviceDisplayInfo;

import java.util.Locale;
import java.util.concurrent.Callable;

/**
 * Tests for usage and quirks of viewport related methods.
 */
public class AwViewportTest extends AwTestBase {

    @MediumTest
    @Feature({"AndroidWebView"})
    public void testTargetDensityDpi() throws Throwable {
        final TestAwContentsClient contentClient = new TestAwContentsClient();
        final AwTestContainerView testContainerView =
                createAwTestContainerViewOnMainSync(contentClient);
        final AwContents awContents = testContainerView.getAwContents();
        AwSettings settings = getAwSettingsOnUiThread(awContents);
        CallbackHelper onPageFinishedHelper = contentClient.getOnPageFinishedHelper();

        final String pageTemplate = "<html><head>" +
                "<meta name='viewport' content='width=device-width, target-densityDpi=%s' />" +
                "</head><body onload='document.title=document.body.clientWidth'></body></html>";
        final String pageDeviceDpi = String.format((Locale)null, pageTemplate, "device-dpi");
        final String pageHighDpi = String.format((Locale)null, pageTemplate, "high-dpi");
        final String pageDpi100 = String.format((Locale)null, pageTemplate, "100");

        settings.setJavaScriptEnabled(true);

        DeviceDisplayInfo deviceInfo =
                DeviceDisplayInfo.create(getInstrumentation().getTargetContext());
        loadDataSync(awContents, onPageFinishedHelper, pageDeviceDpi, "text/html", false);
        int actualWidth = Integer.parseInt(getTitleOnUiThread(awContents));
        assertEquals(deviceInfo.getDisplayWidth(), actualWidth, 10f);

        float displayWidth = (deviceInfo.getDisplayWidth());
        float deviceDpi = (float) (160f * deviceInfo.getDIPScale());

        loadDataSync(awContents, onPageFinishedHelper, pageHighDpi, "text/html", false);
        actualWidth = Integer.parseInt(getTitleOnUiThread(awContents));
        assertEquals(displayWidth * (240f / deviceDpi), actualWidth, 10f);

        loadDataSync(awContents, onPageFinishedHelper, pageDpi100, "text/html", false);
        actualWidth = Integer.parseInt(getTitleOnUiThread(awContents));
        assertEquals(displayWidth * (100f / deviceDpi), actualWidth, 10f);
    }

    @MediumTest
    @Feature({"AndroidWebView"})
    public void testWideViewportInitialScaleDoesNotExpandFixedLayoutWidth() throws Throwable {
        final TestAwContentsClient contentClient = new TestAwContentsClient();
        final AwTestContainerView testContainerView =
                createAwTestContainerViewOnMainSync(contentClient);
        final AwContents awContents = testContainerView.getAwContents();
        AwSettings settings = getAwSettingsOnUiThread(awContents);
        CallbackHelper onPageFinishedHelper = contentClient.getOnPageFinishedHelper();

        final String page = "<html><head>" +
                "<meta name='viewport' content='width=device-width, initial-scale=0.5' />" +
                "</head><body onload='document.title=document.body.clientWidth'></body></html>";

        settings.setJavaScriptEnabled(true);
        settings.setUseWideViewPort(true);

        DeviceDisplayInfo deviceInfo =
                DeviceDisplayInfo.create(getInstrumentation().getTargetContext());
        loadDataSync(awContents, onPageFinishedHelper, page, "text/html", false);
        float displayWidth = (float) (deviceInfo.getDisplayWidth() / deviceInfo.getDIPScale());
        int actualWidth = Integer.parseInt(getTitleOnUiThread(awContents));
        assertEquals(displayWidth, actualWidth, 10f);
        assertEquals(1.0f, getScaleOnUiThread(awContents));
    }

    @MediumTest
    @Feature({"AndroidWebView"})
    public void testZeroValuesQuirk() throws Throwable {
        final TestAwContentsClient contentClient = new TestAwContentsClient();
        final AwTestContainerView testContainerView =
                createAwTestContainerViewOnMainSync(contentClient);
        final AwContents awContents = testContainerView.getAwContents();
        AwSettings settings = getAwSettingsOnUiThread(awContents);
        CallbackHelper onPageFinishedHelper = contentClient.getOnPageFinishedHelper();

        final String page = "<html><head>" +
                "<meta name='viewport' content='width=0, height=0, initial-scale=0.0, " +
                "    minimum-scale=0.0, maximum-scale=0.0' />" +
                "</head><body onload='document.title=document.body.clientWidth'></body></html>";

        settings.setJavaScriptEnabled(true);

        DeviceDisplayInfo deviceInfo =
                DeviceDisplayInfo.create(getInstrumentation().getTargetContext());
        loadDataSync(awContents, onPageFinishedHelper, page, "text/html", false);
        float displayWidth = (float) (deviceInfo.getDisplayWidth() / deviceInfo.getDIPScale());
        int actualWidth = Integer.parseInt(getTitleOnUiThread(awContents));
        assertEquals(displayWidth, actualWidth, 10f);
        assertEquals(1.0f, getScaleOnUiThread(awContents));

        settings.setUseWideViewPort(true);
        loadDataSync(awContents, onPageFinishedHelper, page, "text/html", false);
        actualWidth = Integer.parseInt(getTitleOnUiThread(awContents));
        assertEquals(displayWidth, actualWidth, 10f);
        assertEquals(1.0f, getScaleOnUiThread(awContents));
    }

    @MediumTest
    @Feature({"AndroidWebView"})
    public void testScreenSizeInPhysicalPixelsQuirk() throws Throwable {
        final TestAwContentsClient contentClient = new TestAwContentsClient();
        final AwTestContainerView testContainerView =
                createAwTestContainerViewOnMainSync(contentClient);
        final AwContents awContents = testContainerView.getAwContents();
        AwSettings settings = getAwSettingsOnUiThread(awContents);
        CallbackHelper onPageFinishedHelper = contentClient.getOnPageFinishedHelper();

        settings.setJavaScriptEnabled(true);

        loadUrlSync(awContents, onPageFinishedHelper, "about:blank");

        DeviceDisplayInfo deviceInfo =
                DeviceDisplayInfo.create(getInstrumentation().getTargetContext());
        float dipScale = (float) deviceInfo.getDIPScale();
        float physicalDisplayWidth = deviceInfo.getDisplayWidth();
        float cssDisplayWidth = physicalDisplayWidth / dipScale;
        float physicalDisplayHeight = deviceInfo.getDisplayHeight();
        float cssDisplayHeight = physicalDisplayHeight / dipScale;

        float screenWidth = Integer.parseInt(
                executeJavaScriptAndWaitForResult(awContents, contentClient, "screen.width"));
        assertEquals(physicalDisplayWidth, screenWidth, 10f);
        float screenAvailWidth = Integer.parseInt(
                executeJavaScriptAndWaitForResult(awContents, contentClient, "screen.availWidth"));
        assertEquals(physicalDisplayWidth, screenAvailWidth, 10f);
        float outerWidth = Integer.parseInt(
                executeJavaScriptAndWaitForResult(awContents, contentClient, "outerWidth"));
        float innerWidth = Integer.parseInt(
                executeJavaScriptAndWaitForResult(awContents, contentClient, "innerWidth"));
        assertEquals(innerWidth * dipScale, outerWidth, 10f);
        String deviceWidthEqualsScreenWidth = executeJavaScriptAndWaitForResult(awContents,
                contentClient,
                "matchMedia(\"screen and (device-width:" + (int) screenWidth + "px)\").matches");
        assertEquals("true", deviceWidthEqualsScreenWidth);

        float screenHeight = Integer.parseInt(
                executeJavaScriptAndWaitForResult(awContents, contentClient, "screen.height"));
        assertEquals(physicalDisplayHeight, screenHeight, 10f);
        float screenAvailHeight = Integer.parseInt(
                executeJavaScriptAndWaitForResult(awContents, contentClient, "screen.availHeight"));
        assertEquals(physicalDisplayHeight, screenAvailHeight, 10f);
        float outerHeight = Integer.parseInt(
                executeJavaScriptAndWaitForResult(awContents, contentClient, "outerHeight"));
        float innerHeight = Integer.parseInt(
                executeJavaScriptAndWaitForResult(awContents, contentClient, "innerHeight"));
        assertEquals(innerHeight * dipScale, outerHeight, 10f);
        String deviceHeightEqualsScreenHeight = executeJavaScriptAndWaitForResult(awContents,
                contentClient,
                "matchMedia(\"screen and (device-height:" + (int) screenHeight + "px)\").matches");
        assertEquals("true", deviceHeightEqualsScreenHeight);
    }

    @MediumTest
    @Feature({"AndroidWebView"})
    public void testMetaMergeContentQuirk() throws Throwable {
        final TestAwContentsClient contentClient = new TestAwContentsClient();
        final AwTestContainerView testContainerView =
                createAwTestContainerViewOnMainSync(contentClient);
        final AwContents awContents = testContainerView.getAwContents();
        AwSettings settings = getAwSettingsOnUiThread(awContents);
        CallbackHelper onPageFinishedHelper = contentClient.getOnPageFinishedHelper();

        final int pageWidth = 3000;
        final float pageScale = 1.0f;
        final String page = String.format((Locale)null, "<html><head>" +
                "<meta name='viewport' content='width=%d' />" +
                "<meta name='viewport' content='initial-scale=%.1f' />" +
                "<meta name='viewport' content='user-scalable=0' />" +
                "</head><body onload='document.title=document.body.clientWidth'></body></html>",
                pageWidth, pageScale);

        settings.setJavaScriptEnabled(true);
        settings.setUseWideViewPort(true);
        settings.setBuiltInZoomControls(true);
        settings.setSupportZoom(true);

        loadDataSync(awContents, onPageFinishedHelper, page, "text/html", false);
        int width = Integer.parseInt(getTitleOnUiThread(awContents));
        assertEquals(pageWidth, width);
        assertEquals(pageScale, getScaleOnUiThread(awContents));
        assertEquals(false, canZoomInOnUiThread(awContents));
        assertEquals(false, canZoomOutOnUiThread(awContents));
    }

    @MediumTest
    @Feature({"AndroidWebView"})
    public void testMetaMergeContentQuirkOverrides() throws Throwable {
        final TestAwContentsClient contentClient = new TestAwContentsClient();
        final AwTestContainerView testContainerView =
                createAwTestContainerViewOnMainSync(contentClient);
        final AwContents awContents = testContainerView.getAwContents();
        AwSettings settings = getAwSettingsOnUiThread(awContents);
        CallbackHelper onPageFinishedHelper = contentClient.getOnPageFinishedHelper();

        final int pageWidth = 3000;
        final String page = String.format((Locale)null, "<html><head>" +
                "<meta name='viewport' content='width=device-width' />" +
                "<meta name='viewport' content='width=%d' />" +
                "</head><body onload='document.title=document.body.clientWidth'></body></html>",
                pageWidth);

        settings.setJavaScriptEnabled(true);
        settings.setUseWideViewPort(true);

        loadDataSync(awContents, onPageFinishedHelper, page, "text/html", false);
        int width = Integer.parseInt(getTitleOnUiThread(awContents));
        assertEquals(pageWidth, width);
    }

    @MediumTest
    @Feature({"AndroidWebView"})
    public void testInitialScaleClobberQuirk() throws Throwable {
        final TestAwContentsClient contentClient = new TestAwContentsClient();
        final AwTestContainerView testContainerView =
                createAwTestContainerViewOnMainSync(contentClient);
        final AwContents awContents = testContainerView.getAwContents();
        AwSettings settings = getAwSettingsOnUiThread(awContents);
        CallbackHelper onPageFinishedHelper = contentClient.getOnPageFinishedHelper();

        final String pageTemplate = "<html><head>" +
                "<meta name='viewport' content='initial-scale=%d' />" +
                "</head><body>" +
                "<div style='width:10000px;height:200px'>A big div</div>" +
                "</body></html>";
        final String pageScale4 = String.format((Locale)null, pageTemplate, 4);
        final String page = String.format((Locale)null, pageTemplate, 1);

        // Page scale updates are asynchronous. There is an issue that we can't
        // reliably check, whether the scale as NOT changed (i.e. remains to be 1.0).
        // So we first change the scale to some non-default value, and then wait
        // until it gets back to 1.0.
        int onScaleChangedCallCount = contentClient.getOnScaleChangedHelper().getCallCount();
        loadDataSync(awContents, onPageFinishedHelper, pageScale4, "text/html", false);
        contentClient.getOnScaleChangedHelper().waitForCallback(onScaleChangedCallCount);
        assertEquals(4.0f, getScaleOnUiThread(awContents));
        // The following call to set initial scale will be ignored. However, a temporary
        // page scale change may occur, and this makes the usual onScaleChanged-based workflow
        // flaky. So instead, we are just polling the scale until it becomes 1.0.
        settings.setInitialPageScale(50);
        loadDataSync(awContents, onPageFinishedHelper, page, "text/html", false);
        ensureScaleBecomes(1.0f, awContents);
    }

    @MediumTest
    @Feature({"AndroidWebView"})
    public void testNoUserScalableQuirk() throws Throwable {
        final TestAwContentsClient contentClient = new TestAwContentsClient();
        final AwTestContainerView testContainerView =
                createAwTestContainerViewOnMainSync(contentClient);
        final AwContents awContents = testContainerView.getAwContents();
        CallbackHelper onPageFinishedHelper = contentClient.getOnPageFinishedHelper();

        final String pageScale4 = "<html><head>" +
                "<meta name='viewport' content='initial-scale=4' />" +
                "</head><body>" +
                "<div style='width:10000px;height:200px'>A big div</div>" +
                "</body></html>";
        final String page = "<html><head>" +
                "<meta name='viewport' " +
                "content='width=device-width,initial-scale=2,user-scalable=no' />" +
                "</head><body>" +
                "<div style='width:10000px;height:200px'>A big div</div>" +
                "</body></html>";

        // Page scale updates are asynchronous. There is an issue that we can't
        // reliably check, whether the scale as NOT changed (i.e. remains to be 1.0).
        // So we first change the scale to some non-default value, and then wait
        // until it gets back to 1.0.
        int onScaleChangedCallCount = contentClient.getOnScaleChangedHelper().getCallCount();
        loadDataSync(awContents, onPageFinishedHelper, pageScale4, "text/html", false);
        contentClient.getOnScaleChangedHelper().waitForCallback(onScaleChangedCallCount);
        assertEquals(4.0f, getScaleOnUiThread(awContents));
        onScaleChangedCallCount = contentClient.getOnScaleChangedHelper().getCallCount();
        loadDataSync(awContents, onPageFinishedHelper, page, "text/html", false);
        contentClient.getOnScaleChangedHelper().waitForCallback(onScaleChangedCallCount);
        assertEquals(1.0f, getScaleOnUiThread(awContents));
    }

    private void ensureScaleBecomes(final float targetScale, final AwContents awContents)
            throws Throwable {
        poll(new Callable<Boolean>() {
            @Override
            public Boolean call() throws Exception {
                return targetScale == getScaleOnUiThread(awContents);
            }
        });
    }
}
