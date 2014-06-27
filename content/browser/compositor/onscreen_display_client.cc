// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/compositor/onscreen_display_client.h"

#include "cc/output/output_surface.h"
#include "cc/surfaces/surface_factory.h"
#include "cc/surfaces/surface_manager.h"
#include "content/common/host_shared_bitmap_manager.h"

namespace content {

OnscreenDisplayClient::OnscreenDisplayClient(
    const scoped_refptr<cc::ContextProvider>& onscreen_context_provider,
    scoped_ptr<cc::OutputSurface> software_surface,
    cc::SurfaceManager* manager)
    : manager_(manager),
      onscreen_context_provider_(onscreen_context_provider),
      software_surface_(software_surface.Pass()) {
}

OnscreenDisplayClient::~OnscreenDisplayClient() {
}

void OnscreenDisplayClient::CreateDisplay(cc::SurfaceFactory* factory) {
  display_.reset(new cc::Display(
      this, manager_, factory, HostSharedBitmapManager::current()));
}

scoped_ptr<cc::OutputSurface> OnscreenDisplayClient::CreateOutputSurface() {
  if (!onscreen_context_provider_)
    return software_surface_.Pass();
  return make_scoped_ptr(new cc::OutputSurface(onscreen_context_provider_))
      .Pass();
}

}  // namespace content
