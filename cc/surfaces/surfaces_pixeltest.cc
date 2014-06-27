// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/output/compositor_frame.h"
#include "cc/quads/render_pass.h"
#include "cc/quads/solid_color_draw_quad.h"
#include "cc/quads/surface_draw_quad.h"
#include "cc/surfaces/surface.h"
#include "cc/surfaces/surface_aggregator.h"
#include "cc/surfaces/surface_factory.h"
#include "cc/surfaces/surface_factory_client.h"
#include "cc/surfaces/surface_manager.h"
#include "cc/test/pixel_comparator.h"
#include "cc/test/pixel_test.h"
#include "testing/gtest/include/gtest/gtest.h"

#if !defined(OS_ANDROID)

namespace cc {
namespace {

class EmptySurfaceFactoryClient : public SurfaceFactoryClient {
 public:
  virtual void ReturnResources(
      const ReturnedResourceArray& resources) OVERRIDE {}
};

class SurfacesPixelTest : public RendererPixelTest<GLRenderer> {
 public:
  SurfacesPixelTest() : factory_(&manager_, &client_) {}

 protected:
  SurfaceManager manager_;
  EmptySurfaceFactoryClient client_;
  SurfaceFactory factory_;
};

SharedQuadState* CreateAndAppendTestSharedQuadState(
    RenderPass* render_pass,
    const gfx::Transform& transform,
    const gfx::Size& size) {
  const gfx::Size content_bounds = size;
  const gfx::Rect visible_content_rect = gfx::Rect(size);
  const gfx::Rect clip_rect = gfx::Rect(size);
  bool is_clipped = false;
  float opacity = 1.f;
  const SkXfermode::Mode blend_mode = SkXfermode::kSrcOver_Mode;
  SharedQuadState* shared_state = render_pass->CreateAndAppendSharedQuadState();
  shared_state->SetAll(transform,
                       content_bounds,
                       visible_content_rect,
                       clip_rect,
                       is_clipped,
                       opacity,
                       blend_mode,
                       0);
  return shared_state;
}

// Draws a very simple frame with no surface references.
TEST_F(SurfacesPixelTest, DrawSimpleFrame) {
  gfx::Rect rect(device_viewport_size_);
  RenderPass::Id id(1, 1);
  scoped_ptr<RenderPass> pass = RenderPass::Create();
  pass->SetNew(id, rect, rect, gfx::Transform());

  CreateAndAppendTestSharedQuadState(
      pass.get(), gfx::Transform(), device_viewport_size_);

  scoped_ptr<SolidColorDrawQuad> color_quad = SolidColorDrawQuad::Create();
  bool force_anti_aliasing_off = false;
  color_quad->SetNew(pass->shared_quad_state_list.back(),
                     rect,
                     rect,
                     SK_ColorGREEN,
                     force_anti_aliasing_off);
  pass->quad_list.push_back(color_quad.PassAs<DrawQuad>());

  scoped_ptr<DelegatedFrameData> delegated_frame_data(new DelegatedFrameData);
  delegated_frame_data->render_pass_list.push_back(pass.Pass());

  scoped_ptr<CompositorFrame> root_frame(new CompositorFrame);
  root_frame->delegated_frame_data = delegated_frame_data.Pass();

  SurfaceId root_surface_id = factory_.Create(device_viewport_size_);
  factory_.SubmitFrame(root_surface_id, root_frame.Pass());

  SurfaceAggregator aggregator(&manager_, resource_provider_.get());
  scoped_ptr<CompositorFrame> aggregated_frame =
      aggregator.Aggregate(root_surface_id);
  factory_.Destroy(root_surface_id);

  bool discard_alpha = false;
  ExactPixelComparator pixel_comparator(discard_alpha);
  RenderPassList* pass_list =
      &aggregated_frame->delegated_frame_data->render_pass_list;
  EXPECT_TRUE(RunPixelTest(pass_list,
                           base::FilePath(FILE_PATH_LITERAL("green.png")),
                           pixel_comparator));
}

// Draws a frame with simple surface embedding.
TEST_F(SurfacesPixelTest, DrawSimpleAggregatedFrame) {
  gfx::Size child_size(200, 100);
  SurfaceId child_surface_id = factory_.Create(child_size);
  SurfaceId root_surface_id = factory_.Create(device_viewport_size_);

  {
    gfx::Rect rect(device_viewport_size_);
    RenderPass::Id id(1, 1);
    scoped_ptr<RenderPass> pass = RenderPass::Create();
    pass->SetNew(id, rect, rect, gfx::Transform());

    CreateAndAppendTestSharedQuadState(
        pass.get(), gfx::Transform(), device_viewport_size_);

    scoped_ptr<SurfaceDrawQuad> surface_quad = SurfaceDrawQuad::Create();
    surface_quad->SetNew(pass->shared_quad_state_list.back(),
                         gfx::Rect(child_size),
                         gfx::Rect(child_size),
                         child_surface_id);
    pass->quad_list.push_back(surface_quad.PassAs<DrawQuad>());

    scoped_ptr<SolidColorDrawQuad> color_quad = SolidColorDrawQuad::Create();
    bool force_anti_aliasing_off = false;
    color_quad->SetNew(pass->shared_quad_state_list.back(),
                       rect,
                       rect,
                       SK_ColorYELLOW,
                       force_anti_aliasing_off);
    pass->quad_list.push_back(color_quad.PassAs<DrawQuad>());

    scoped_ptr<DelegatedFrameData> delegated_frame_data(new DelegatedFrameData);
    delegated_frame_data->render_pass_list.push_back(pass.Pass());

    scoped_ptr<CompositorFrame> root_frame(new CompositorFrame);
    root_frame->delegated_frame_data = delegated_frame_data.Pass();

    factory_.SubmitFrame(root_surface_id, root_frame.Pass());
  }

  {
    gfx::Rect rect(child_size);
    RenderPass::Id id(1, 1);
    scoped_ptr<RenderPass> pass = RenderPass::Create();
    pass->SetNew(id, rect, rect, gfx::Transform());

    CreateAndAppendTestSharedQuadState(
        pass.get(), gfx::Transform(), child_size);

    scoped_ptr<SolidColorDrawQuad> color_quad = SolidColorDrawQuad::Create();
    bool force_anti_aliasing_off = false;
    color_quad->SetNew(pass->shared_quad_state_list.back(),
                       rect,
                       rect,
                       SK_ColorBLUE,
                       force_anti_aliasing_off);
    pass->quad_list.push_back(color_quad.PassAs<DrawQuad>());

    scoped_ptr<DelegatedFrameData> delegated_frame_data(new DelegatedFrameData);
    delegated_frame_data->render_pass_list.push_back(pass.Pass());

    scoped_ptr<CompositorFrame> child_frame(new CompositorFrame);
    child_frame->delegated_frame_data = delegated_frame_data.Pass();

    factory_.SubmitFrame(child_surface_id, child_frame.Pass());
  }

  SurfaceAggregator aggregator(&manager_, resource_provider_.get());
  scoped_ptr<CompositorFrame> aggregated_frame =
      aggregator.Aggregate(root_surface_id);

  bool discard_alpha = false;
  ExactPixelComparator pixel_comparator(discard_alpha);
  RenderPassList* pass_list =
      &aggregated_frame->delegated_frame_data->render_pass_list;
  EXPECT_TRUE(RunPixelTest(pass_list,
                           base::FilePath(FILE_PATH_LITERAL("blue_yellow.png")),
                           pixel_comparator));
  factory_.Destroy(root_surface_id);
  factory_.Destroy(child_surface_id);
}

// Tests a surface quad that has a non-identity transform into its pass.
TEST_F(SurfacesPixelTest, DrawAggregatedFrameWithSurfaceTransforms) {
  gfx::Size child_size(100, 200);
  gfx::Size quad_size(100, 100);
  // Structure:
  // root (200x200) -> left_child (100x200 @ 0x0,
  //                   right_child (100x200 @ 0x100)
  //   left_child -> top_green_quad (100x100 @ 0x0),
  //                 bottom_blue_quad (100x100 @ 0x100)
  //   right_child -> top_blue_quad (100x100 @ 0x0),
  //                  bottom_green_quad (100x100 @ 0x100)
  SurfaceId left_child_id = factory_.Create(child_size);
  SurfaceId right_child_id = factory_.Create(child_size);
  SurfaceId root_surface_id = factory_.Create(device_viewport_size_);

  {
    gfx::Rect rect(device_viewport_size_);
    RenderPass::Id id(1, 1);
    scoped_ptr<RenderPass> pass = RenderPass::Create();
    pass->SetNew(id, rect, rect, gfx::Transform());

    gfx::Transform surface_transform;
    CreateAndAppendTestSharedQuadState(
        pass.get(), surface_transform, device_viewport_size_);

    scoped_ptr<SurfaceDrawQuad> left_surface_quad = SurfaceDrawQuad::Create();
    left_surface_quad->SetNew(pass->shared_quad_state_list.back(),
                              gfx::Rect(child_size),
                              gfx::Rect(child_size),
                              left_child_id);
    pass->quad_list.push_back(left_surface_quad.PassAs<DrawQuad>());

    surface_transform.Translate(100, 0);
    CreateAndAppendTestSharedQuadState(
        pass.get(), surface_transform, device_viewport_size_);

    scoped_ptr<SurfaceDrawQuad> right_surface_quad = SurfaceDrawQuad::Create();
    right_surface_quad->SetNew(pass->shared_quad_state_list.back(),
                               gfx::Rect(child_size),
                               gfx::Rect(child_size),
                               right_child_id);
    pass->quad_list.push_back(right_surface_quad.PassAs<DrawQuad>());

    scoped_ptr<DelegatedFrameData> delegated_frame_data(new DelegatedFrameData);
    delegated_frame_data->render_pass_list.push_back(pass.Pass());

    scoped_ptr<CompositorFrame> root_frame(new CompositorFrame);
    root_frame->delegated_frame_data = delegated_frame_data.Pass();

    factory_.SubmitFrame(root_surface_id, root_frame.Pass());
  }

  {
    gfx::Rect rect(child_size);
    RenderPass::Id id(1, 1);
    scoped_ptr<RenderPass> pass = RenderPass::Create();
    pass->SetNew(id, rect, rect, gfx::Transform());

    CreateAndAppendTestSharedQuadState(
        pass.get(), gfx::Transform(), child_size);

    scoped_ptr<SolidColorDrawQuad> top_color_quad =
        SolidColorDrawQuad::Create();
    bool force_anti_aliasing_off = false;
    top_color_quad->SetNew(pass->shared_quad_state_list.back(),
                           gfx::Rect(quad_size),
                           gfx::Rect(quad_size),
                           SK_ColorGREEN,
                           force_anti_aliasing_off);
    pass->quad_list.push_back(top_color_quad.PassAs<DrawQuad>());

    scoped_ptr<SolidColorDrawQuad> bottom_color_quad =
        SolidColorDrawQuad::Create();
    bottom_color_quad->SetNew(pass->shared_quad_state_list.back(),
                              gfx::Rect(0, 100, 100, 100),
                              gfx::Rect(0, 100, 100, 100),
                              SK_ColorBLUE,
                              force_anti_aliasing_off);
    pass->quad_list.push_back(bottom_color_quad.PassAs<DrawQuad>());

    scoped_ptr<DelegatedFrameData> delegated_frame_data(new DelegatedFrameData);
    delegated_frame_data->render_pass_list.push_back(pass.Pass());

    scoped_ptr<CompositorFrame> child_frame(new CompositorFrame);
    child_frame->delegated_frame_data = delegated_frame_data.Pass();

    factory_.SubmitFrame(left_child_id, child_frame.Pass());
  }

  {
    gfx::Rect rect(child_size);
    RenderPass::Id id(1, 1);
    scoped_ptr<RenderPass> pass = RenderPass::Create();
    pass->SetNew(id, rect, rect, gfx::Transform());

    CreateAndAppendTestSharedQuadState(
        pass.get(), gfx::Transform(), child_size);

    scoped_ptr<SolidColorDrawQuad> top_color_quad =
        SolidColorDrawQuad::Create();
    bool force_anti_aliasing_off = false;
    top_color_quad->SetNew(pass->shared_quad_state_list.back(),
                           gfx::Rect(quad_size),
                           gfx::Rect(quad_size),
                           SK_ColorBLUE,
                           force_anti_aliasing_off);
    pass->quad_list.push_back(top_color_quad.PassAs<DrawQuad>());

    scoped_ptr<SolidColorDrawQuad> bottom_color_quad =
        SolidColorDrawQuad::Create();
    bottom_color_quad->SetNew(pass->shared_quad_state_list.back(),
                              gfx::Rect(0, 100, 100, 100),
                              gfx::Rect(0, 100, 100, 100),
                              SK_ColorGREEN,
                              force_anti_aliasing_off);
    pass->quad_list.push_back(bottom_color_quad.PassAs<DrawQuad>());

    scoped_ptr<DelegatedFrameData> delegated_frame_data(new DelegatedFrameData);
    delegated_frame_data->render_pass_list.push_back(pass.Pass());

    scoped_ptr<CompositorFrame> child_frame(new CompositorFrame);
    child_frame->delegated_frame_data = delegated_frame_data.Pass();

    factory_.SubmitFrame(right_child_id, child_frame.Pass());
  }

  SurfaceAggregator aggregator(&manager_, resource_provider_.get());
  scoped_ptr<CompositorFrame> aggregated_frame =
      aggregator.Aggregate(root_surface_id);

  bool discard_alpha = false;
  ExactPixelComparator pixel_comparator(discard_alpha);
  RenderPassList* pass_list =
      &aggregated_frame->delegated_frame_data->render_pass_list;
  EXPECT_TRUE(RunPixelTest(
      pass_list,
      base::FilePath(FILE_PATH_LITERAL("four_blue_green_checkers.png")),
      pixel_comparator));

  factory_.Destroy(root_surface_id);
  factory_.Destroy(left_child_id);
  factory_.Destroy(right_child_id);
}

}  // namespace
}  // namespace cc

#endif  // !defined(OS_ANDROID)
