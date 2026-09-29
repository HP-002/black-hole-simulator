#pragma once

#include <glm/glm.hpp>

#include <array>
#include <vector>

// Cubemap face order and (s, t) axes follow the OpenGL spec:
// +X, -X, +Y, -Y, +Z, -Z
struct CubeCoord {
    int face;
    glm::vec2 uv; // [0, 1]
};

CubeCoord cubeCoord(const glm::vec3& direction);
glm::vec3 cubeDirection(int face, const glm::vec2& uv); // unit length

struct CubeFaces {
    int size = 0;
    std::array<std::vector<unsigned char>, 6> rgb; // row t, column s
};

// Random stars plus a faint galactic band
CubeFaces generateStarField(int size, int starCount, unsigned seed);
