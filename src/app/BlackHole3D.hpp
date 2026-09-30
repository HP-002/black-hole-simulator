#pragma once

#include "core/Camera.hpp"
#include "core/Shader.hpp"
#include "core/Window.hpp"
#include "render/Bloom.hpp"
#include "render/Cubemap.hpp"
#include "render/FullscreenTriangle.hpp"
#include "render/Texture2D.hpp"

// Fly-through view: one bent light ray per pixel, traced by a compute
// shader into an image, with a thin accretion disk
class BlackHole3D {
  public:
    BlackHole3D();

    void update(const Window& window, float deltaTime);
    void render(glm::ivec2 framebufferSize);

    float renderScale() const { return renderScale_; }
    float exposure() const { return exposure_; }
    bool paused() const { return paused_; }

  private:
    Shader tracer_;
    Shader toneMap_;
    Texture2D image_;
    Bloom bloom_;
    FullscreenTriangle screen_;
    Cubemap skybox_;
    Camera camera_;
    float rs_ = 1.0f;
    float renderScale_ = 1.0f; // traced pixels per screen pixel, per axis
    float exposure_ = 1.0f;    // HDR multiplier before tone mapping
    double time_ = 0.0;        // coordinate time in rs / c; double for long runs
    bool paused_ = false;
};
