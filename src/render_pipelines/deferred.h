#ifndef REI_DEFERRED_H
#define REI_DEFERRED_H

#include "render_pipeline_base.h"

namespace rei {

// FIXME lazyness
namespace d3d {
class Renderer;
}

// Forwared decl
namespace deferred {
struct ViewportData;
struct SceneData;
} // namespace deferred

class DeferredPipeline
    : public SimplexPipeline<deferred::ViewportData, deferred::SceneData, d3d::Renderer> {
  // FIXME lazyness
  using Renderer = d3d::Renderer;

public:
  DeferredPipeline(RendererPtr renderer);

  virtual ViewportHandle register_viewport(ViewportConfig conf) override;
  virtual void remove_viewport(ViewportHandle viewport) override {}
  virtual void transform_viewport(ViewportHandle handle, const Camera& camera) override;

  virtual SceneHandle register_scene(SceneConfig conf) override;
  virtual void remove_scene(SceneHandle scene) override {}

  virtual void update_model(SceneHandle scene, const Model& model) override;
  virtual void add_model(SceneHandle scene, ModelHandle model) override {}

  virtual void render(ViewportHandle viewport, SceneHandle scene) override;

private:
  // FIXME should be material handle
  ShaderHandle m_default_shader;
  ShaderHandle m_lighting_shader;

  BufferHandle m_per_render_buffer;
};

} // namespace rei

#endif