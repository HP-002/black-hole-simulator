#include "render/Texture2D.hpp"

Texture2D::Texture2D(GLenum internalFormat) : format_(internalFormat) {}

Texture2D::~Texture2D() {
    glDeleteTextures(1, &texture_);
}

void Texture2D::resize(glm::ivec2 size) {
    if (size == size_) {
        return;
    }
    // Immutable storage: recreate
    glDeleteTextures(1, &texture_);
    glGenTextures(1, &texture_);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexStorage2D(GL_TEXTURE_2D, 1, format_, size.x, size.y);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    size_ = size;
}

void Texture2D::bind(int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, texture_);
}

void Texture2D::bindImage(int unit, GLenum access) const {
    glBindImageTexture(unit, texture_, 0, GL_FALSE, 0, access, format_);
}
