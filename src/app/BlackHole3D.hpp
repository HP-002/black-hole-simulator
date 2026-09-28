#pragma once

#include "core/Camera.hpp"
#include "core/Shader.hpp"
#include "core/Window.hpp"
#include "render/Cubemap.hpp"
#include "render/FullscreenTriangle.hpp"

// Fly-through view: one camera ray per pixel against a star-field skybox
class BlackHole3D {
  public:
    BlackHole3D();

    void update(const Window& window, float deltaTime);
    void render(float aspectRatio) const;

  private:
    Shader shader_;
    FullscreenTriangle screen_;
    Cubemap skybox_;
    Camera camera_;
    float rs_ = 1.0f;
};
