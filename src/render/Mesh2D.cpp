#include "render/Mesh2D.hpp"

#include <cstddef>

Mesh2D::Mesh2D() {
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

Mesh2D::~Mesh2D() {
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &vbo_);
}

void Mesh2D::clear() {
    vertices_.clear();
    primitives_.clear();
}

void Mesh2D::add(GLenum mode, const std::vector<glm::vec2>& points,
                 const glm::vec3& color) {
    primitives_.push_back({mode, static_cast<GLint>(vertices_.size()),
                           static_cast<GLsizei>(points.size())});
    for (const glm::vec2& p : points) {
        vertices_.push_back({p, color});
    }
}

void Mesh2D::upload() {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(Vertex),
                 vertices_.data(), GL_DYNAMIC_DRAW);
}

void Mesh2D::draw() const {
    glBindVertexArray(vao_);
    for (const Primitive& p : primitives_) {
        glDrawArrays(p.mode, p.first, p.count);
    }
    glBindVertexArray(0);
}
