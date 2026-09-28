#include "render/Cubemap.hpp"

Cubemap::Cubemap(const CubeFaces& faces) {
    glGenTextures(1, &texture_);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture_);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (int f = 0; f < 6; ++f) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + f, 0, GL_RGB8, faces.size,
                     faces.size, 0, GL_RGB, GL_UNSIGNED_BYTE,
                     faces.rgb[f].data());
    }
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

Cubemap::~Cubemap() {
    glDeleteTextures(1, &texture_);
}

void Cubemap::bind(int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture_);
}
