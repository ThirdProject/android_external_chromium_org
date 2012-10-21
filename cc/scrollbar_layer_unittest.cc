// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"

#include "cc/scrollbar_layer.h"

#include "cc/scrollbar_animation_controller.h"
#include "cc/scrollbar_layer_impl.h"
#include "cc/single_thread_proxy.h"
#include "cc/test/fake_web_scrollbar_theme_geometry.h"
#include "cc/tree_synchronizer.h"
#include "testing/gtest/include/gtest/gtest.h"
#include <public/WebScrollbar.h>
#include <public/WebScrollbarThemeGeometry.h>
#include <public/WebScrollbarThemePainter.h>

using namespace cc;

namespace {

class FakeWebScrollbar : public WebKit::WebScrollbar {
public:
    static scoped_ptr<FakeWebScrollbar> create() { return make_scoped_ptr(new FakeWebScrollbar()); }

    // WebScrollbar implementation
    virtual bool isOverlay() const OVERRIDE { return false; }
    virtual int value() const OVERRIDE { return 0; }
    virtual WebKit::WebPoint location() const OVERRIDE { return WebKit::WebPoint(); }
    virtual WebKit::WebSize size() const OVERRIDE { return WebKit::WebSize(); }
    virtual bool enabled() const OVERRIDE { return true; }
    virtual int maximum() const OVERRIDE { return 0; }
    virtual int totalSize() const OVERRIDE { return 0; }
    virtual bool isScrollViewScrollbar() const OVERRIDE { return false; }
    virtual bool isScrollableAreaActive() const OVERRIDE { return true; }
    virtual void getTickmarks(WebKit::WebVector<WebKit::WebRect>&) const OVERRIDE { }
    virtual ScrollbarControlSize controlSize() const OVERRIDE { return WebScrollbar::RegularScrollbar; }
    virtual ScrollbarPart pressedPart() const OVERRIDE { return WebScrollbar::NoPart; }
    virtual ScrollbarPart hoveredPart() const OVERRIDE { return WebScrollbar::NoPart; }
    virtual ScrollbarOverlayStyle scrollbarOverlayStyle() const OVERRIDE { return WebScrollbar::ScrollbarOverlayStyleDefault; }
    virtual bool isCustomScrollbar() const OVERRIDE { return false; }
    virtual Orientation orientation() const OVERRIDE { return WebScrollbar::Horizontal; }
};

TEST(ScrollbarLayerChromiumTest, resolveScrollLayerPointer)
{
    DebugScopedSetImplThread impl;

    WebKit::WebScrollbarThemePainter painter;

    {
        scoped_ptr<WebKit::WebScrollbar> scrollbar(FakeWebScrollbar::create());
        scoped_refptr<LayerChromium> layerTreeRoot = LayerChromium::create();
        scoped_refptr<LayerChromium> child1 = LayerChromium::create();
        scoped_refptr<LayerChromium> child2 = ScrollbarLayerChromium::create(scrollbar.Pass(), painter, WebKit::FakeWebScrollbarThemeGeometry::create(), child1->id());
        layerTreeRoot->addChild(child1);
        layerTreeRoot->addChild(child2);

        scoped_ptr<CCLayerImpl> ccLayerTreeRoot = TreeSynchronizer::synchronizeTrees(layerTreeRoot.get(), scoped_ptr<CCLayerImpl>(), 0);

        CCLayerImpl* ccChild1 = ccLayerTreeRoot->children()[0];
        CCScrollbarLayerImpl* ccChild2 = static_cast<CCScrollbarLayerImpl*>(ccLayerTreeRoot->children()[1]);

        EXPECT_TRUE(ccChild1->scrollbarAnimationController());
        EXPECT_EQ(ccChild1->horizontalScrollbarLayer(), ccChild2);
    }

    { // another traverse order
        scoped_ptr<WebKit::WebScrollbar> scrollbar(FakeWebScrollbar::create());
        scoped_refptr<LayerChromium> layerTreeRoot = LayerChromium::create();
        scoped_refptr<LayerChromium> child2 = LayerChromium::create();
        scoped_refptr<LayerChromium> child1 = ScrollbarLayerChromium::create(scrollbar.Pass(), painter, WebKit::FakeWebScrollbarThemeGeometry::create(), child2->id());
        layerTreeRoot->addChild(child1);
        layerTreeRoot->addChild(child2);

        scoped_ptr<CCLayerImpl> ccLayerTreeRoot = TreeSynchronizer::synchronizeTrees(layerTreeRoot.get(), scoped_ptr<CCLayerImpl>(), 0);

        CCScrollbarLayerImpl* ccChild1 = static_cast<CCScrollbarLayerImpl*>(ccLayerTreeRoot->children()[0]);
        CCLayerImpl* ccChild2 = ccLayerTreeRoot->children()[1];

        EXPECT_TRUE(ccChild2->scrollbarAnimationController());
        EXPECT_EQ(ccChild2->horizontalScrollbarLayer(), ccChild1);
    }
}

TEST(ScrollbarLayerChromiumTest, scrollOffsetSynchronization)
{
    DebugScopedSetImplThread impl;

    WebKit::WebScrollbarThemePainter painter;

    scoped_ptr<WebKit::WebScrollbar> scrollbar(FakeWebScrollbar::create());
    scoped_refptr<LayerChromium> layerTreeRoot = LayerChromium::create();
    scoped_refptr<LayerChromium> contentLayer = LayerChromium::create();
    scoped_refptr<LayerChromium> scrollbarLayer = ScrollbarLayerChromium::create(scrollbar.Pass(), painter, WebKit::FakeWebScrollbarThemeGeometry::create(), layerTreeRoot->id());
    layerTreeRoot->addChild(contentLayer);
    layerTreeRoot->addChild(scrollbarLayer);

    layerTreeRoot->setScrollPosition(IntPoint(10, 20));
    layerTreeRoot->setMaxScrollPosition(IntSize(30, 50));
    contentLayer->setBounds(IntSize(100, 200));

    scoped_ptr<CCLayerImpl> ccLayerTreeRoot = TreeSynchronizer::synchronizeTrees(layerTreeRoot.get(), scoped_ptr<CCLayerImpl>(), 0);

    CCScrollbarLayerImpl* ccScrollbarLayer = static_cast<CCScrollbarLayerImpl*>(ccLayerTreeRoot->children()[1]);

    EXPECT_EQ(10, ccScrollbarLayer->currentPos());
    EXPECT_EQ(100, ccScrollbarLayer->totalSize());
    EXPECT_EQ(30, ccScrollbarLayer->maximum());

    layerTreeRoot->setScrollPosition(IntPoint(100, 200));
    layerTreeRoot->setMaxScrollPosition(IntSize(300, 500));
    contentLayer->setBounds(IntSize(1000, 2000));

    CCScrollbarAnimationController* scrollbarController = ccLayerTreeRoot->scrollbarAnimationController();
    ccLayerTreeRoot = TreeSynchronizer::synchronizeTrees(layerTreeRoot.get(), ccLayerTreeRoot.Pass(), 0);
    EXPECT_EQ(scrollbarController, ccLayerTreeRoot->scrollbarAnimationController());

    EXPECT_EQ(100, ccScrollbarLayer->currentPos());
    EXPECT_EQ(1000, ccScrollbarLayer->totalSize());
    EXPECT_EQ(300, ccScrollbarLayer->maximum());

    ccLayerTreeRoot->scrollBy(FloatSize(12, 34));

    EXPECT_EQ(112, ccScrollbarLayer->currentPos());
    EXPECT_EQ(1000, ccScrollbarLayer->totalSize());
    EXPECT_EQ(300, ccScrollbarLayer->maximum());
}

}
