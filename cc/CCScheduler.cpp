// Copyright 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"

#include "CCScheduler.h"

#include "TraceEvent.h"

namespace WebCore {

CCScheduler::CCScheduler(CCSchedulerClient* client, PassOwnPtr<CCFrameRateController> frameRateController)
    : m_client(client)
    , m_frameRateController(frameRateController)
    , m_updateMoreResourcesPending(false)
{
    ASSERT(m_client);
    m_frameRateController->setClient(this);
    m_frameRateController->setActive(m_stateMachine.vsyncCallbackNeeded());
}

CCScheduler::~CCScheduler()
{
    m_frameRateController->setActive(false);
}

void CCScheduler::setCanBeginFrame(bool can)
{
    m_stateMachine.setCanBeginFrame(can);
    processScheduledActions();
}

void CCScheduler::setVisible(bool visible)
{
    m_stateMachine.setVisible(visible);
    processScheduledActions();
}

void CCScheduler::setNeedsCommit()
{
    m_stateMachine.setNeedsCommit();
    processScheduledActions();
}

void CCScheduler::setNeedsForcedCommit()
{
    m_stateMachine.setNeedsForcedCommit();
    processScheduledActions();
}

void CCScheduler::setNeedsRedraw()
{
    m_stateMachine.setNeedsRedraw();
    processScheduledActions();
}

void CCScheduler::setNeedsForcedRedraw()
{
    m_stateMachine.setNeedsForcedRedraw();
    processScheduledActions();
}

void CCScheduler::setMainThreadNeedsLayerTextures()
{
    m_stateMachine.setMainThreadNeedsLayerTextures();
    processScheduledActions();
}

void CCScheduler::beginFrameComplete()
{
    TRACE_EVENT0("cc", "CCScheduler::beginFrameComplete");
    m_stateMachine.beginFrameComplete();
    processScheduledActions();
}

void CCScheduler::beginFrameAborted()
{
    TRACE_EVENT0("cc", "CCScheduler::beginFrameAborted");
    m_stateMachine.beginFrameAborted();
    processScheduledActions();
}

void CCScheduler::setMaxFramesPending(int maxFramesPending)
{
    m_frameRateController->setMaxFramesPending(maxFramesPending);
}

void CCScheduler::didSwapBuffersComplete()
{
    TRACE_EVENT0("cc", "CCScheduler::didSwapBuffersComplete");
    m_frameRateController->didFinishFrame();
}

void CCScheduler::didLoseContext()
{
    TRACE_EVENT0("cc", "CCScheduler::didLoseContext");
    m_frameRateController->didAbortAllPendingFrames();
    m_stateMachine.didLoseContext();
    processScheduledActions();
}

void CCScheduler::didRecreateContext()
{
    TRACE_EVENT0("cc", "CCScheduler::didRecreateContext");
    m_stateMachine.didRecreateContext();
    processScheduledActions();
}

void CCScheduler::setTimebaseAndInterval(double timebase, double intervalSeconds)
{
    m_frameRateController->setTimebaseAndInterval(timebase, intervalSeconds);
}

void CCScheduler::vsyncTick()
{
    if (m_updateMoreResourcesPending) {
        m_updateMoreResourcesPending = false;
        m_stateMachine.beginUpdateMoreResourcesComplete(m_client->hasMoreResourceUpdates());
    }
    TRACE_EVENT0("cc", "CCScheduler::vsyncTick");

    m_stateMachine.didEnterVSync();
    processScheduledActions();
    m_stateMachine.didLeaveVSync();
}

CCSchedulerStateMachine::Action CCScheduler::nextAction()
{
    m_stateMachine.setCanDraw(m_client->canDraw());
    return m_stateMachine.nextAction();
}

void CCScheduler::processScheduledActions()
{
    // Early out so we don't spam TRACE_EVENTS with useless processScheduledActions.
    if (nextAction() == CCSchedulerStateMachine::ACTION_NONE) {
        m_frameRateController->setActive(m_stateMachine.vsyncCallbackNeeded());
        return;
    }

    // This function can re-enter itself. For example, draw may call
    // setNeedsCommit. Proceeed with caution.
    CCSchedulerStateMachine::Action action;
    do {
        action = nextAction();
        m_stateMachine.updateState(action);
        TRACE_EVENT1("cc", "CCScheduler::processScheduledActions()", "action", action);

        switch (action) {
        case CCSchedulerStateMachine::ACTION_NONE:
            break;
        case CCSchedulerStateMachine::ACTION_BEGIN_FRAME:
            m_client->scheduledActionBeginFrame();
            break;
        case CCSchedulerStateMachine::ACTION_BEGIN_UPDATE_MORE_RESOURCES:
            if (m_client->hasMoreResourceUpdates()) {
                m_client->scheduledActionUpdateMoreResources(m_frameRateController->nextTickTimeIfActivated());
                m_updateMoreResourcesPending = true;
            } else
                m_stateMachine.beginUpdateMoreResourcesComplete(false);
            break;
        case CCSchedulerStateMachine::ACTION_COMMIT:
            m_client->scheduledActionCommit();
            break;
        case CCSchedulerStateMachine::ACTION_DRAW_IF_POSSIBLE: {
            CCScheduledActionDrawAndSwapResult result = m_client->scheduledActionDrawAndSwapIfPossible();
            m_stateMachine.didDrawIfPossibleCompleted(result.didDraw);
            if (result.didSwap)
                m_frameRateController->didBeginFrame();
            break;
        }
        case CCSchedulerStateMachine::ACTION_DRAW_FORCED: {
            CCScheduledActionDrawAndSwapResult result = m_client->scheduledActionDrawAndSwapForced();
            if (result.didSwap)
                m_frameRateController->didBeginFrame();
            break;
        } case CCSchedulerStateMachine::ACTION_BEGIN_CONTEXT_RECREATION:
            m_client->scheduledActionBeginContextRecreation();
            break;
        case CCSchedulerStateMachine::ACTION_ACQUIRE_LAYER_TEXTURES_FOR_MAIN_THREAD:
            m_client->scheduledActionAcquireLayerTexturesForMainThread();
            break;
        }
    } while (action != CCSchedulerStateMachine::ACTION_NONE);

    // Activate or deactivate the frame rate controller.
    m_frameRateController->setActive(m_stateMachine.vsyncCallbackNeeded());
}

}
