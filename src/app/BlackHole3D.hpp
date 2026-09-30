#pragma once

#include "core/Camera.hpp"
#include "core/Shader.hpp"
#include "core/Window.hpp"
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

  private:
    Shader tracer_;
    Shader blit_;
    Texture2D image_;
    FullscreenTriangle screen_;
    Cubemap skybox_;
    Camera camera_;
    float rs_ = 1.0f;
    float renderScale_ = 1.0f; // traced pixels per screen pixel, per axis
};
