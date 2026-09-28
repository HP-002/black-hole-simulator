#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <vector>

// Colored 2D primitives batched into one vertex buffer
class Mesh2D {
  public:
    Mesh2D();
    ~Mesh2D();

    // Non-copyable: it uniquely owns a VAO and VBO.
    Mesh2D(const Mesh2D&) = delete;
    Mesh2D& operator=(const Mesh2D&) = delete;

    void clear();
    void add(GLenum mode, const std::vector<glm::vec2>& points,
             const glm::vec3& color);
    // Send added primitives to the GPU
    void upload();
    void draw() const;

  private:
    struct Vertex {
        glm::vec2 position;
        glm::vec3 color;
    };
    struct Primitive {
        GLenum mode;
        GLint first;
        GLsizei count;
    };

    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    std::vector<Vertex> vertices_;
    std::vector<Primitive> primitives_;
};
