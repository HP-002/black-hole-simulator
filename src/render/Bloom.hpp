#pragma once

#include "core/Shader.hpp"
#include "render/Texture2D.hpp"

#include <memory>
#include <vector>

// Glow around bright pixels: bright-pass, a chain of half-size blurred
// levels, then summed back up
class Bloom {
  public:
    Bloom();

    // Blurred bright parts of hdr, at half its size
    const Texture2D& apply(const Texture2D& hdr);

  private:
    Shader down_;
    Shader up_;
    std::vector<std::unique_ptr<Texture2D>> levels_; // halving each step
};
