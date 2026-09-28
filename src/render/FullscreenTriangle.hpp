#pragma once

#include <GL/glew.h>

// One triangle covering the viewport; vertices come from gl_VertexID
class FullscreenTriangle {
  public:
    FullscreenTriangle();
    ~FullscreenTriangle();

    // Non-copyable: it uniquely owns a VAO.
    FullscreenTriangle(const FullscreenTriangle&) = delete;
    FullscreenTriangle& operator=(const FullscreenTriangle&) = delete;

    void draw() const;

  private:
    GLuint vao_ = 0; // core profile needs one bound
};
