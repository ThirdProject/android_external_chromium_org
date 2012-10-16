// Copyright 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"

#include "cc/layer.h"

#include "cc/active_animation.h"
#include "cc/animation_events.h"
#include "CCLayerAnimationController.h"
#include "CCLayerImpl.h"
#include "CCLayerTreeHost.h"
#include "CCSettings.h"
#include <public/WebAnimationDelegate.h>
#include <public/WebLayerScrollClient.h>
#include <public/WebSize.h>

using namespace std;
using WebKit::WebTransformationMatrix;

namespace cc {

static int s_nextLayerId = 1;

scoped_refptr<LayerChromium> LayerChromium::create()
{
    return make_scoped_refptr(new LayerChromium());
}

LayerChromium::LayerChromium()
    : m_needsDisplay(false)
    , m_stackingOrderChanged(false)
    , m_layerId(s_nextLayerId++)
    , m_parent(0)
    , m_layerTreeHost(0)
    , m_layerAnimationController(CCLayerAnimationController::create(this))
    , m_scrollable(false)
    , m_shouldScrollOnMainThread(false)
    , m_haveWheelEventHandlers(false)
    , m_nonFastScrollableRegionChanged(false)
    , m_anchorPoint(0.5, 0.5)
    , m_backgroundColor(0)
    , m_debugBorderColor(0)
    , m_debugBorderWidth(0)
    , m_opacity(1.0)
    , m_anchorPointZ(0)
    , m_isContainerForFixedPositionLayers(false)
    , m_fixedToContainerLayer(false)
    , m_isDrawable(false)
    , m_masksToBounds(false)
    , m_contentsOpaque(false)
    , m_doubleSided(true)
    , m_useLCDText(false)
    , m_preserves3D(false)
    , m_useParentBackfaceVisibility(false)
    , m_drawCheckerboardForMissingTiles(false)
    , m_forceRenderSurface(false)
    , m_replicaLayer(0)
    , m_drawOpacity(0)
    , m_drawOpacityIsAnimating(false)
    , m_renderTarget(0)
    , m_drawTransformIsAnimating(false)
    , m_screenSpaceTransformIsAnimating(false)
    , m_contentsScale(1.0)
    , m_boundsContainPageScale(false)
    , m_layerAnimationDelegate(0)
    , m_layerScrollClient(0)
{
    if (m_layerId < 0) {
        s_nextLayerId = 1;
        m_layerId = s_nextLayerId++;
    }
}

LayerChromium::~LayerChromium()
{
    // Our parent should be holding a reference to us so there should be no
    // way for us to be destroyed while we still have a parent.
    ASSERT(!parent());

    // Remove the parent reference from all children.
    removeAllChildren();
}

void LayerChromium::setUseLCDText(bool useLCDText)
{
    m_useLCDText = useLCDText;
}

void LayerChromium::setLayerTreeHost(CCLayerTreeHost* host)
{
    if (m_layerTreeHost == host)
        return;

    m_layerTreeHost = host;

    for (size_t i = 0; i < m_children.size(); ++i)
        m_children[i]->setLayerTreeHost(host);

    if (m_maskLayer)
        m_maskLayer->setLayerTreeHost(host);
    if (m_replicaLayer)
        m_replicaLayer->setLayerTreeHost(host);

    // If this layer already has active animations, the host needs to be notified.
    if (host && m_layerAnimationController->hasActiveAnimation())
        host->didAddAnimation();
}

void LayerChromium::setNeedsCommit()
{
    if (m_layerTreeHost)
        m_layerTreeHost->setNeedsCommit();
}

IntRect LayerChromium::layerRectToContentRect(const WebKit::WebRect& layerRect)
{
    float widthScale = static_cast<float>(contentBounds().width()) / bounds().width();
    float heightScale = static_cast<float>(contentBounds().height()) / bounds().height();
    FloatRect contentRect(layerRect.x, layerRect.y, layerRect.width, layerRect.height);
    contentRect.scale(widthScale, heightScale);
    return enclosingIntRect(contentRect);
}

void LayerChromium::setParent(LayerChromium* layer)
{
    ASSERT(!layer || !layer->hasAncestor(this));
    m_parent = layer;
    setLayerTreeHost(m_parent ? m_parent->layerTreeHost() : 0);
}

bool LayerChromium::hasAncestor(LayerChromium* ancestor) const
{
    for (LayerChromium* layer = parent(); layer; layer = layer->parent()) {
        if (layer == ancestor)
            return true;
    }
    return false;
}

void LayerChromium::addChild(scoped_refptr<LayerChromium> child)
{
    insertChild(child, numChildren());
}

void LayerChromium::insertChild(scoped_refptr<LayerChromium> child, size_t index)
{
    index = min(index, m_children.size());
    child->removeFromParent();
    child->setParent(this);
    child->m_stackingOrderChanged = true;

    LayerList::iterator iter = m_children.begin();
    m_children.insert(iter + index, child);
    setNeedsCommit();
}

void LayerChromium::removeFromParent()
{
    if (m_parent)
        m_parent->removeChild(this);
}

void LayerChromium::removeChild(LayerChromium* child)
{
    for (LayerList::iterator iter = m_children.begin(); iter != m_children.end(); ++iter)
    {
        if (*iter != child)
            continue;

        child->setParent(0);
        m_children.erase(iter);
        setNeedsCommit();
        return;
    }
}

void LayerChromium::replaceChild(LayerChromium* reference, scoped_refptr<LayerChromium> newLayer)
{
    ASSERT_ARG(reference, reference);
    ASSERT_ARG(reference, reference->parent() == this);

    if (reference == newLayer)
        return;

    int referenceIndex = indexOfChild(reference);
    if (referenceIndex == -1) {
        ASSERT_NOT_REACHED();
        return;
    }

    reference->removeFromParent();

    if (newLayer) {
        newLayer->removeFromParent();
        insertChild(newLayer, referenceIndex);
    }
}

int LayerChromium::indexOfChild(const LayerChromium* reference)
{
    for (size_t i = 0; i < m_children.size(); i++) {
        if (m_children[i] == reference)
            return i;
    }
    return -1;
}

void LayerChromium::setBounds(const IntSize& size)
{
    if (bounds() == size)
        return;

    bool firstResize = bounds().isEmpty() && !size.isEmpty();

    m_bounds = size;

    if (firstResize)
        setNeedsDisplay();
    else
        setNeedsCommit();
}

LayerChromium* LayerChromium::rootLayer()
{
    LayerChromium* layer = this;
    while (layer->parent())
        layer = layer->parent();
    return layer;
}

void LayerChromium::removeAllChildren()
{
    while (m_children.size()) {
        LayerChromium* layer = m_children[0].get();
        ASSERT(layer->parent());
        layer->removeFromParent();
    }
}

void LayerChromium::setChildren(const LayerList& children)
{
    if (children == m_children)
        return;

    removeAllChildren();
    size_t listSize = children.size();
    for (size_t i = 0; i < listSize; i++)
        addChild(children[i]);
}

void LayerChromium::setAnchorPoint(const FloatPoint& anchorPoint)
{
    if (m_anchorPoint == anchorPoint)
        return;
    m_anchorPoint = anchorPoint;
    setNeedsCommit();
}

void LayerChromium::setAnchorPointZ(float anchorPointZ)
{
    if (m_anchorPointZ == anchorPointZ)
        return;
    m_anchorPointZ = anchorPointZ;
    setNeedsCommit();
}

void LayerChromium::setBackgroundColor(SkColor backgroundColor)
{
    if (m_backgroundColor == backgroundColor)
        return;
    m_backgroundColor = backgroundColor;
    setNeedsCommit();
}

IntSize LayerChromium::contentBounds() const
{
    return bounds();
}

void LayerChromium::setMasksToBounds(bool masksToBounds)
{
    if (m_masksToBounds == masksToBounds)
        return;
    m_masksToBounds = masksToBounds;
    setNeedsCommit();
}

void LayerChromium::setMaskLayer(LayerChromium* maskLayer)
{
    if (m_maskLayer == maskLayer)
        return;
    if (m_maskLayer)
        m_maskLayer->setLayerTreeHost(0);
    m_maskLayer = maskLayer;
    if (m_maskLayer) {
        m_maskLayer->setLayerTreeHost(m_layerTreeHost);
        m_maskLayer->setIsMask(true);
    }
    setNeedsCommit();
}

void LayerChromium::setReplicaLayer(LayerChromium* layer)
{
    if (m_replicaLayer == layer)
        return;
    if (m_replicaLayer)
        m_replicaLayer->setLayerTreeHost(0);
    m_replicaLayer = layer;
    if (m_replicaLayer)
        m_replicaLayer->setLayerTreeHost(m_layerTreeHost);
    setNeedsCommit();
}

void LayerChromium::setFilters(const WebKit::WebFilterOperations& filters)
{
    if (m_filters == filters)
        return;
    m_filters = filters;
    setNeedsCommit();
    if (!filters.isEmpty())
        CCLayerTreeHost::setNeedsFilterContext(true);
}

void LayerChromium::setBackgroundFilters(const WebKit::WebFilterOperations& backgroundFilters)
{
    if (m_backgroundFilters == backgroundFilters)
        return;
    m_backgroundFilters = backgroundFilters;
    setNeedsCommit();
    if (!backgroundFilters.isEmpty())
        CCLayerTreeHost::setNeedsFilterContext(true);
}

bool LayerChromium::needsDisplay() const
{
    return m_needsDisplay;
}

void LayerChromium::setOpacity(float opacity)
{
    if (m_opacity == opacity)
        return;
    m_opacity = opacity;
    setNeedsCommit();
}

bool LayerChromium::opacityIsAnimating() const
{
    return m_layerAnimationController->isAnimatingProperty(CCActiveAnimation::Opacity);
}

void LayerChromium::setContentsOpaque(bool opaque)
{
    if (m_contentsOpaque == opaque)
        return;
    m_contentsOpaque = opaque;
    setNeedsDisplay();
}

void LayerChromium::setPosition(const FloatPoint& position)
{
    if (m_position == position)
        return;
    m_position = position;
    setNeedsCommit();
}

void LayerChromium::setSublayerTransform(const WebTransformationMatrix& sublayerTransform)
{
    if (m_sublayerTransform == sublayerTransform)
        return;
    m_sublayerTransform = sublayerTransform;
    setNeedsCommit();
}

void LayerChromium::setTransform(const WebTransformationMatrix& transform)
{
    if (m_transform == transform)
        return;
    m_transform = transform;
    setNeedsCommit();
}

bool LayerChromium::transformIsAnimating() const
{
    return m_layerAnimationController->isAnimatingProperty(CCActiveAnimation::Transform);
}

void LayerChromium::setScrollPosition(const IntPoint& scrollPosition)
{
    if (m_scrollPosition == scrollPosition)
        return;
    m_scrollPosition = scrollPosition;
    if (m_layerScrollClient)
        m_layerScrollClient->didScroll();
    setNeedsCommit();
}

void LayerChromium::setMaxScrollPosition(const IntSize& maxScrollPosition)
{
    if (m_maxScrollPosition == maxScrollPosition)
        return;
    m_maxScrollPosition = maxScrollPosition;
    setNeedsCommit();
}

void LayerChromium::setScrollable(bool scrollable)
{
    if (m_scrollable == scrollable)
        return;
    m_scrollable = scrollable;
    setNeedsCommit();
}

void LayerChromium::setShouldScrollOnMainThread(bool shouldScrollOnMainThread)
{
    if (m_shouldScrollOnMainThread == shouldScrollOnMainThread)
        return;
    m_shouldScrollOnMainThread = shouldScrollOnMainThread;
    setNeedsCommit();
}

void LayerChromium::setHaveWheelEventHandlers(bool haveWheelEventHandlers)
{
    if (m_haveWheelEventHandlers == haveWheelEventHandlers)
        return;
    m_haveWheelEventHandlers = haveWheelEventHandlers;
    setNeedsCommit();
}

void LayerChromium::setNonFastScrollableRegion(const Region& region)
{
    if (m_nonFastScrollableRegion == region)
        return;
    m_nonFastScrollableRegion = region;
    m_nonFastScrollableRegionChanged = true;
    setNeedsCommit();
}

void LayerChromium::setDrawCheckerboardForMissingTiles(bool checkerboard)
{
    if (m_drawCheckerboardForMissingTiles == checkerboard)
        return;
    m_drawCheckerboardForMissingTiles = checkerboard;
    setNeedsCommit();
}

void LayerChromium::setForceRenderSurface(bool force)
{
    if (m_forceRenderSurface == force)
        return;
    m_forceRenderSurface = force;
    setNeedsCommit();
}

void LayerChromium::setImplTransform(const WebTransformationMatrix& transform)
{
    if (m_implTransform == transform)
        return;
    m_implTransform = transform;
    setNeedsCommit();
}

void LayerChromium::setDoubleSided(bool doubleSided)
{
    if (m_doubleSided == doubleSided)
        return;
    m_doubleSided = doubleSided;
    setNeedsCommit();
}

void LayerChromium::setIsDrawable(bool isDrawable)
{
    if (m_isDrawable == isDrawable)
        return;

    m_isDrawable = isDrawable;
    setNeedsCommit();
}

LayerChromium* LayerChromium::parent() const
{
    return m_parent;
}

void LayerChromium::setNeedsDisplayRect(const FloatRect& dirtyRect)
{
    m_updateRect.unite(dirtyRect);

    // Simply mark the contents as dirty. For non-root layers, the call to
    // setNeedsCommit will schedule a fresh compositing pass.
    // For the root layer, setNeedsCommit has no effect.
    if (!dirtyRect.isEmpty())
        m_needsDisplay = true;

    setNeedsCommit();
}

bool LayerChromium::descendantIsFixedToContainerLayer() const
{
    for (size_t i = 0; i < m_children.size(); ++i) {
        if (m_children[i]->fixedToContainerLayer() || m_children[i]->descendantIsFixedToContainerLayer())
            return true;
    }
    return false;
}

void LayerChromium::setIsContainerForFixedPositionLayers(bool isContainerForFixedPositionLayers)
{
    if (m_isContainerForFixedPositionLayers == isContainerForFixedPositionLayers)
        return;
    m_isContainerForFixedPositionLayers = isContainerForFixedPositionLayers;

    if (m_layerTreeHost && m_layerTreeHost->commitRequested())
        return;

    // Only request a commit if we have a fixed positioned descendant.
    if (descendantIsFixedToContainerLayer())
        setNeedsCommit();
}

void LayerChromium::setFixedToContainerLayer(bool fixedToContainerLayer)
{
    if (m_fixedToContainerLayer == fixedToContainerLayer)
        return;
    m_fixedToContainerLayer = fixedToContainerLayer;
    setNeedsCommit();
}

void LayerChromium::pushPropertiesTo(CCLayerImpl* layer)
{
    layer->setAnchorPoint(m_anchorPoint);
    layer->setAnchorPointZ(m_anchorPointZ);
    layer->setBackgroundColor(m_backgroundColor);
    layer->setBounds(m_bounds);
    layer->setContentBounds(contentBounds());
    layer->setDebugBorderColor(m_debugBorderColor);
    layer->setDebugBorderWidth(m_debugBorderWidth);
    layer->setDebugName(m_debugName);
    layer->setDoubleSided(m_doubleSided);
    layer->setDrawCheckerboardForMissingTiles(m_drawCheckerboardForMissingTiles);
    layer->setForceRenderSurface(m_forceRenderSurface);
    layer->setDrawsContent(drawsContent());
    layer->setFilters(filters());
    layer->setBackgroundFilters(backgroundFilters());
    layer->setUseLCDText(m_useLCDText);
    layer->setMasksToBounds(m_masksToBounds);
    layer->setScrollable(m_scrollable);
    layer->setShouldScrollOnMainThread(m_shouldScrollOnMainThread);
    layer->setHaveWheelEventHandlers(m_haveWheelEventHandlers);
    // Copying a Region is more expensive than most layer properties, since it involves copying two Vectors that may be
    // arbitrarily large depending on page content, so we only push the property if it's changed.
    if (m_nonFastScrollableRegionChanged) {
        layer->setNonFastScrollableRegion(m_nonFastScrollableRegion);
        m_nonFastScrollableRegionChanged = false;
    }
    layer->setContentsOpaque(m_contentsOpaque);
    if (!opacityIsAnimating())
        layer->setOpacity(m_opacity);
    layer->setPosition(m_position);
    layer->setIsContainerForFixedPositionLayers(m_isContainerForFixedPositionLayers);
    layer->setFixedToContainerLayer(m_fixedToContainerLayer);
    layer->setPreserves3D(preserves3D());
    layer->setUseParentBackfaceVisibility(m_useParentBackfaceVisibility);
    layer->setScrollPosition(m_scrollPosition);
    layer->setMaxScrollPosition(m_maxScrollPosition);
    layer->setSublayerTransform(m_sublayerTransform);
    if (!transformIsAnimating())
        layer->setTransform(m_transform);

    // If the main thread commits multiple times before the impl thread actually draws, then damage tracking
    // will become incorrect if we simply clobber the updateRect here. The CCLayerImpl's updateRect needs to
    // accumulate (i.e. union) any update changes that have occurred on the main thread.
    m_updateRect.uniteIfNonZero(layer->updateRect());
    layer->setUpdateRect(m_updateRect);

    layer->setScrollDelta(layer->scrollDelta() - layer->sentScrollDelta());
    layer->setSentScrollDelta(IntSize());

    layer->setStackingOrderChanged(m_stackingOrderChanged);

    if (maskLayer())
        maskLayer()->pushPropertiesTo(layer->maskLayer());
    if (replicaLayer())
        replicaLayer()->pushPropertiesTo(layer->replicaLayer());

    m_layerAnimationController->pushAnimationUpdatesTo(layer->layerAnimationController());

    // Reset any state that should be cleared for the next update.
    m_stackingOrderChanged = false;
    m_updateRect = FloatRect();
}

scoped_ptr<CCLayerImpl> LayerChromium::createCCLayerImpl()
{
    return CCLayerImpl::create(m_layerId);
}

bool LayerChromium::drawsContent() const
{
    return m_isDrawable;
}

bool LayerChromium::needMoreUpdates()
{
    return false;
}

bool LayerChromium::needsContentsScale() const
{
    return false;
}

void LayerChromium::setDebugBorderColor(SkColor color)
{
    m_debugBorderColor = color;
    setNeedsCommit();
}

void LayerChromium::setDebugBorderWidth(float width)
{
    m_debugBorderWidth = width;
    setNeedsCommit();
}

void LayerChromium::setDebugName(const std::string& debugName)
{
    m_debugName = debugName;
    setNeedsCommit();
}

void LayerChromium::setContentsScale(float contentsScale)
{
    if (!needsContentsScale() || m_contentsScale == contentsScale)
        return;
    m_contentsScale = contentsScale;

    setNeedsDisplay();
}

void LayerChromium::setBoundsContainPageScale(bool boundsContainPageScale)
{
    for (size_t i = 0; i < m_children.size(); ++i)
        m_children[i]->setBoundsContainPageScale(boundsContainPageScale);

    if (boundsContainPageScale == m_boundsContainPageScale)
        return;

    m_boundsContainPageScale = boundsContainPageScale;
    setNeedsDisplay();
}

void LayerChromium::createRenderSurface()
{
    ASSERT(!m_renderSurface);
    m_renderSurface = make_scoped_ptr(new RenderSurfaceChromium(this));
    setRenderTarget(this);
}

bool LayerChromium::descendantDrawsContent()
{
    for (size_t i = 0; i < m_children.size(); ++i) {
        if (m_children[i]->drawsContent() || m_children[i]->descendantDrawsContent())
            return true;
    }
    return false;
}

int LayerChromium::id() const
{
    return m_layerId;
}

float LayerChromium::opacity() const
{
    return m_opacity;
}

void LayerChromium::setOpacityFromAnimation(float opacity)
{
    // This is called due to an ongoing accelerated animation. Since this animation is
    // also being run on the impl thread, there is no need to request a commit to push
    // this value over, so set the value directly rather than calling setOpacity.
    m_opacity = opacity;
}

const WebKit::WebTransformationMatrix& LayerChromium::transform() const
{
    return m_transform;
}

void LayerChromium::setTransformFromAnimation(const WebTransformationMatrix& transform)
{
    // This is called due to an ongoing accelerated animation. Since this animation is
    // also being run on the impl thread, there is no need to request a commit to push
    // this value over, so set this value directly rather than calling setTransform.
    m_transform = transform;
}

bool LayerChromium::addAnimation(scoped_ptr <CCActiveAnimation> animation)
{
    // WebCore currently assumes that accelerated animations will start soon
    // after the animation is added. However we cannot guarantee that if we do
    // not have a layerTreeHost that will setNeedsCommit().
    if (!m_layerTreeHost)
        return false;

    if (!CCSettings::acceleratedAnimationEnabled())
        return false;

    m_layerAnimationController->addAnimation(animation.Pass());
    if (m_layerTreeHost) {
        m_layerTreeHost->didAddAnimation();
        setNeedsCommit();
    }
    return true;
}

void LayerChromium::pauseAnimation(int animationId, double timeOffset)
{
    m_layerAnimationController->pauseAnimation(animationId, timeOffset);
    setNeedsCommit();
}

void LayerChromium::removeAnimation(int animationId)
{
    m_layerAnimationController->removeAnimation(animationId);
    setNeedsCommit();
}

void LayerChromium::suspendAnimations(double monotonicTime)
{
    m_layerAnimationController->suspendAnimations(monotonicTime);
    setNeedsCommit();
}

void LayerChromium::resumeAnimations(double monotonicTime)
{
    m_layerAnimationController->resumeAnimations(monotonicTime);
    setNeedsCommit();
}

void LayerChromium::setLayerAnimationController(scoped_ptr<CCLayerAnimationController> layerAnimationController)
{
    m_layerAnimationController = layerAnimationController.Pass();
    if (m_layerAnimationController) {
        m_layerAnimationController->setClient(this);
        m_layerAnimationController->setForceSync();
    }
    setNeedsCommit();
}

scoped_ptr<CCLayerAnimationController> LayerChromium::releaseLayerAnimationController()
{
    scoped_ptr<CCLayerAnimationController> toReturn = m_layerAnimationController.Pass();
    m_layerAnimationController = CCLayerAnimationController::create(this);
    return toReturn.Pass();
}

bool LayerChromium::hasActiveAnimation() const
{
    return m_layerAnimationController->hasActiveAnimation();
}

void LayerChromium::notifyAnimationStarted(const CCAnimationEvent& event, double wallClockTime)
{
    m_layerAnimationController->notifyAnimationStarted(event);
    if (m_layerAnimationDelegate)
        m_layerAnimationDelegate->notifyAnimationStarted(wallClockTime);
}

void LayerChromium::notifyAnimationFinished(double wallClockTime)
{
    if (m_layerAnimationDelegate)
        m_layerAnimationDelegate->notifyAnimationFinished(wallClockTime);
}

Region LayerChromium::visibleContentOpaqueRegion() const
{
    if (contentsOpaque())
        return visibleContentRect();
    return Region();
}

ScrollbarLayerChromium* LayerChromium::toScrollbarLayerChromium()
{
    return 0;
}

void sortLayers(std::vector<scoped_refptr<LayerChromium> >::iterator, std::vector<scoped_refptr<LayerChromium> >::iterator, void*)
{
    // Currently we don't use z-order to decide what to paint, so there's no need to actually sort LayerChromiums.
}

}
