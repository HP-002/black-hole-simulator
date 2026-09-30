#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

// Single-level 2D texture, sampled with linear filtering and written as an
// image by compute shaders
class Texture2D {
  public:
    explicit Texture2D(GLenum internalFormat);
    ~Texture2D();

    // Non-copyable: it uniquely owns a GL texture.
    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;

    // Reallocates only when the size changes; contents undefined after
    void resize(glm::ivec2 size);
    glm::ivec2 size() const { return size_; }

    void bind(int unit) const;
    void bindImage(int unit, GLenum access) const;

  private:
    GLuint texture_ = 0;
    GLenum format_;
    glm::ivec2 size_{0};
};
