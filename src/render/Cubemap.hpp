#pragma once

#include "render/StarField.hpp"

#include <GL/glew.h>

class Cubemap {
  public:
    explicit Cubemap(const CubeFaces& faces);
    ~Cubemap();

    // Non-copyable: it uniquely owns a GL texture.
    Cubemap(const Cubemap&) = delete;
    Cubemap& operator=(const Cubemap&) = delete;

    void bind(int unit) const;

  private:
    GLuint texture_ = 0;
};
