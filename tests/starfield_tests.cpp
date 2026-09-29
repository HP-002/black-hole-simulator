// Unit tests for render/StarField

#include "render/StarField.hpp"

#include <cmath>
#include <cstdio>
#include <random>

namespace {

int failures = 0;

void check(bool condition, const char* what) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", what);
        ++failures;
    }
}

bool approxEqual(const glm::vec3& a, const glm::vec3& b, float eps = 1e-5f) {
    return glm::length(a - b) <= eps;
}

void faceCentersAreAxes() {
    const glm::vec3 axes[6] = {{1, 0, 0},  {-1, 0, 0}, {0, 1, 0},
                               {0, -1, 0}, {0, 0, 1},  {0, 0, -1}};
    bool ok = true;
    for (int f = 0; f < 6; ++f) {
        ok = ok && approxEqual(cubeDirection(f, glm::vec2(0.5f)), axes[f]);
        const CubeCoord c = cubeCoord(axes[f]);
        ok = ok && c.face == f &&
             approxEqual(glm::vec3(c.uv, 0.0f), glm::vec3(0.5f, 0.5f, 0.0f));
    }
    check(ok, "face centers map to +X, -X, +Y, -Y, +Z, -Z");
}

void matchesGlSpecTable() {
    // s = 1 or t = 1 edges, from the GL cube map selection table
    const float k = 1.0f / std::sqrt(2.0f);
    check(approxEqual(cubeDirection(0, {1.0f, 0.5f}), {k, 0.0f, -k}),
          "+X: s grows toward -z");
    check(approxEqual(cubeDirection(0, {0.5f, 1.0f}), {k, -k, 0.0f}),
          "+X: t grows toward -y");
    check(approxEqual(cubeDirection(2, {0.5f, 1.0f}), {0.0f, k, k}),
          "+Y: t grows toward +z");
    check(approxEqual(cubeDirection(3, {0.5f, 1.0f}), {0.0f, -k, -k}),
          "-Y: t grows toward -z");
    check(approxEqual(cubeDirection(5, {1.0f, 0.5f}), {-k, 0.0f, -k}),
          "-Z: s grows toward -x");
}

void roundTrip() {
    std::mt19937 rng(1);
    std::uniform_real_distribution<float> unit(-1.0f, 1.0f);
    bool ok = true;
    for (int i = 0; i < 10000; ++i) {
        const glm::vec3 d =
            glm::normalize(glm::vec3(unit(rng), unit(rng), unit(rng)) + 1e-3f);
        const CubeCoord c = cubeCoord(d);
        ok = ok && c.uv.x >= 0.0f && c.uv.x <= 1.0f && c.uv.y >= 0.0f &&
             c.uv.y <= 1.0f && approxEqual(cubeDirection(c.face, c.uv), d);
    }
    check(ok, "direction -> face/uv -> direction round trip");
}

void starFieldIsDeterministicAndLit() {
    const CubeFaces a = generateStarField(64, 500, 3);
    const CubeFaces b = generateStarField(64, 500, 3);
    const CubeFaces c = generateStarField(64, 500, 4);

    bool sized = a.size == 64;
    int bright = 0;
    for (int f = 0; f < 6; ++f) {
        sized = sized && a.rgb[f].size() == 64u * 64u * 3u;
        for (unsigned char v : a.rgb[f]) {
            bright += v > 100;
        }
    }
    check(sized, "faces are size x size RGB");
    check(a.rgb == b.rgb, "same seed gives the same sky");
    check(a.rgb != c.rgb, "different seed gives a different sky");
    check(bright > 50, "stars are visible");
}

} // namespace

int main() {
    faceCentersAreAxes();
    matchesGlSpecTable();
    roundTrip();
    starFieldIsDeterministicAndLit();

    if (failures == 0) {
        std::printf("All star field tests passed\n");
    }
    return failures == 0 ? 0 : 1;
}
