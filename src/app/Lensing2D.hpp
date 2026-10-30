#pragma once

#include "core/Shader.hpp"
#include "core/Window.hpp"
#include "render/Mesh2D.hpp"

// Top-down view of a parallel light beam bending around a spinning black
// hole, in its equator
class Lensing2D {
  public:
    Lensing2D();

    void update(const Window& window, float deltaTime);
    void render(float aspectRatio) const;

    float spin() const { return static_cast<float>(spin_); }

  private:
    void rebuild();

    Shader shader_;
    Mesh2D mesh_;
    double rs_ = 1.0;
    double spin_;  // a / M; + turns counterclockwise on screen
    double beamOffset_ = 0.0;
    bool dirty_ = true;
};
