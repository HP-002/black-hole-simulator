#include "render/StarField.hpp"

#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>
#include <random>

namespace {

constexpr float kStarSigma = 0.5f; // texels
constexpr float kBandWidth = 0.12f;
constexpr float kBandBrightness = 0.06f;

const glm::vec3 kBandNormal = glm::normalize(glm::vec3(0.3f, 1.0f, 0.2f));
const glm::vec3 kBandColor(0.5f, 0.55f, 0.8f);
const glm::vec3 kCoolStar(1.0f, 0.75f, 0.55f);
const glm::vec3 kHotStar(0.7f, 0.8f, 1.0f);

struct Star {
    glm::vec3 direction;
    glm::vec3 color;
};

std::vector<Star> randomStars(int count, unsigned seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);

    std::vector<Star> stars;
    stars.reserve(count);
    for (int i = 0; i < count; ++i) {
        // Uniform on the sphere
        const float z = 2.0f * unit(rng) - 1.0f;
        const float phi = glm::two_pi<float>() * unit(rng);
        const float ring = std::sqrt(1.0f - z * z);
        const glm::vec3 direction(ring * std::cos(phi), ring * std::sin(phi),
                                  z);

        // Mostly dim, a few bright
        const float brightness = 0.1f + 0.9f * std::pow(unit(rng), 4.0f);
        const glm::vec3 tint = glm::mix(kCoolStar, kHotStar, unit(rng));
        stars.push_back({direction, tint * brightness});
    }
    return stars;
}

void splat(std::vector<glm::vec3>& face, int size, const glm::vec2& uv,
           const glm::vec3& color) {
    const glm::vec2 center = uv * static_cast<float>(size) - 0.5f;
    const int cx = static_cast<int>(std::lround(center.x));
    const int cy = static_cast<int>(std::lround(center.y));
    for (int y = cy - 1; y <= cy + 1; ++y) {
        for (int x = cx - 1; x <= cx + 1; ++x) {
            if (x < 0 || y < 0 || x >= size || y >= size) {
                continue;
            }
            const glm::vec2 d = glm::vec2(x, y) - center;
            const float w =
                std::exp(-glm::dot(d, d) / (2.0f * kStarSigma * kStarSigma));
            face[y * size + x] += color * w;
        }
    }
}

} // namespace

CubeCoord cubeCoord(const glm::vec3& d) {
    const glm::vec3 a = glm::abs(d);
    int face = 0;
    float sc = 0.0f;
    float tc = 0.0f;
    float ma = 0.0f;
    if (a.x >= a.y && a.x >= a.z) {
        face = d.x > 0.0f ? 0 : 1;
        sc = d.x > 0.0f ? -d.z : d.z;
        tc = -d.y;
        ma = a.x;
    } else if (a.y >= a.z) {
        face = d.y > 0.0f ? 2 : 3;
        sc = d.x;
        tc = d.y > 0.0f ? d.z : -d.z;
        ma = a.y;
    } else {
        face = d.z > 0.0f ? 4 : 5;
        sc = d.z > 0.0f ? d.x : -d.x;
        tc = -d.y;
        ma = a.z;
    }
    return {face, glm::vec2(sc / ma + 1.0f, tc / ma + 1.0f) * 0.5f};
}

glm::vec3 cubeDirection(int face, const glm::vec2& uv) {
    const float sc = 2.0f * uv.x - 1.0f;
    const float tc = 2.0f * uv.y - 1.0f;
    glm::vec3 d(0.0f);
    switch (face) {
    case 0:
        d = glm::vec3(1.0f, -tc, -sc);
        break;
    case 1:
        d = glm::vec3(-1.0f, -tc, sc);
        break;
    case 2:
        d = glm::vec3(sc, 1.0f, tc);
        break;
    case 3:
        d = glm::vec3(sc, -1.0f, -tc);
        break;
    case 4:
        d = glm::vec3(sc, -tc, 1.0f);
        break;
    default:
        d = glm::vec3(-sc, -tc, -1.0f);
        break;
    }
    return glm::normalize(d);
}

CubeFaces generateStarField(int size, int starCount, unsigned seed) {
    const std::vector<Star> stars = randomStars(starCount, seed);

    CubeFaces faces;
    faces.size = size;
    std::vector<glm::vec3> light(static_cast<size_t>(size) * size);
    for (int f = 0; f < 6; ++f) {
        for (int y = 0; y < size; ++y) {
            for (int x = 0; x < size; ++x) {
                const glm::vec2 uv =
                    (glm::vec2(x, y) + 0.5f) / static_cast<float>(size);
                const float h = glm::dot(cubeDirection(f, uv), kBandNormal);
                light[y * size + x] =
                    kBandColor * kBandBrightness *
                    std::exp(-h * h / (2.0f * kBandWidth * kBandWidth));
            }
        }
        for (const Star& star : stars) {
            const CubeCoord c = cubeCoord(star.direction);
            if (c.face == f) {
                splat(light, size, c.uv, star.color);
            }
        }

        std::vector<unsigned char>& rgb = faces.rgb[f];
        rgb.resize(light.size() * 3);
        for (size_t i = 0; i < light.size(); ++i) {
            for (int ch = 0; ch < 3; ++ch) {
                const float v = std::clamp(light[i][ch], 0.0f, 1.0f);
                rgb[i * 3 + ch] = static_cast<unsigned char>(v * 255.0f + 0.5f);
            }
        }
    }
    return faces;
}
