#include "app/Lensing2D.hpp"

#include "physics/Kerr.hpp"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <vector>

namespace {

constexpr float kViewHalfHeight = 10.0f; // world units
constexpr double kBeamStartX = -30.0;
constexpr double kEscapeRadius = 40.0;
constexpr int kRayCount = 41;
constexpr double kRaySpacing = 0.25;
constexpr double kMinRs = 0.25;
constexpr double kMaxRs = 3.0;
constexpr double kMassRate = 0.5;  // log(rs) per second
constexpr double kBeamSpeed = 2.0; // world units per second
constexpr double kStartSpin = 0.6;  // a / M
constexpr double kMaxSpin = 0.998;  // Thorne's limit for real holes
constexpr double kSpinRate = 0.4;   // a / M per second
constexpr int kCircleSegments = 128;

const glm::vec3 kEscapedColor(1.0f, 0.8f, 0.3f);
const glm::vec3 kCapturedColor(0.9f, 0.25f, 0.2f);
const glm::vec3 kUnfinishedColor(1.0f, 1.0f, 1.0f);
const glm::vec3 kHorizonColor(0.0f, 0.0f, 0.0f);
const glm::vec3 kHorizonEdgeColor(1.0f, 0.5f, 0.1f);
const glm::vec3 kErgosphereColor(0.6f, 0.3f, 0.7f);
const glm::vec3 kProgradeOrbitColor(0.3f, 0.7f, 0.8f);
const glm::vec3 kRetrogradeOrbitColor(0.4f, 0.5f, 0.8f);

// Closed ring without a repeated endpoint (for GL_LINE_LOOP)
std::vector<glm::vec2> circle(float radius) {
    std::vector<glm::vec2> points;
    for (int i = 0; i < kCircleSegments; ++i) {
        const float angle =
            glm::two_pi<float>() * static_cast<float>(i) / kCircleSegments;
        points.emplace_back(radius * std::cos(angle), radius * std::sin(angle));
    }
    return points;
}

// Center plus closed ring (for GL_TRIANGLE_FAN)
std::vector<glm::vec2> disk(float radius) {
    std::vector<glm::vec2> points = circle(radius);
    points.push_back(points.front());
    points.insert(points.begin(), glm::vec2(0.0f));
    return points;
}

glm::vec3 fateColor(RayFate fate) {
    switch (fate) {
    case RayFate::Captured:
        return kCapturedColor;
    case RayFate::Escaped:
        return kEscapedColor;
    case RayFate::Unfinished:
        break;
    }
    return kUnfinishedColor;
}

// Screen (x, y) -> equator of a hole spinning about +y, seen from above
glm::dvec3 toWorld(glm::dvec2 p) { return glm::dvec3(p.x, 0.0, -p.y); }
glm::vec2 toScreen(const glm::dvec3& p) {
    return glm::vec2(static_cast<float>(p.x), static_cast<float>(-p.z));
}

} // namespace

Lensing2D::Lensing2D()
    : shader_(BHS_ASSET_DIR "/shaders/mesh2d.vert",
              BHS_ASSET_DIR "/shaders/mesh2d.frag"),
      spin_(kStartSpin) {}

void Lensing2D::update(const Window& window, float deltaTime) {
    if (window.isKeyPressed(GLFW_KEY_UP)) {
        rs_ = std::fmin(rs_ * std::exp(kMassRate * deltaTime), kMaxRs);
        dirty_ = true;
    }
    if (window.isKeyPressed(GLFW_KEY_DOWN)) {
        rs_ = std::fmax(rs_ * std::exp(-kMassRate * deltaTime), kMinRs);
        dirty_ = true;
    }
    if (window.isKeyPressed(GLFW_KEY_W)) {
        beamOffset_ =
            std::fmin(beamOffset_ + kBeamSpeed * deltaTime, kViewHalfHeight);
        dirty_ = true;
    }
    if (window.isKeyPressed(GLFW_KEY_S)) {
        beamOffset_ =
            std::fmax(beamOffset_ - kBeamSpeed * deltaTime, -kViewHalfHeight);
        dirty_ = true;
    }
    if (window.isKeyPressed(GLFW_KEY_RIGHT_BRACKET)) {
        spin_ = std::fmin(spin_ + kSpinRate * deltaTime, kMaxSpin);
        dirty_ = true;
    }
    if (window.isKeyPressed(GLFW_KEY_LEFT_BRACKET)) {
        spin_ = std::fmax(spin_ - kSpinRate * deltaTime, -kMaxSpin);
        dirty_ = true;
    }
    if (window.wasKeyPressed(GLFW_KEY_0)) {
        spin_ = 0.0;
        dirty_ = true;
    }

    if (dirty_) {
        rebuild();
        dirty_ = false;
    }
}

void Lensing2D::render(float aspectRatio) const {
    const float halfWidth = kViewHalfHeight * aspectRatio;
    const glm::mat4 projection =
        glm::ortho(-halfWidth, halfWidth, -kViewHalfHeight, kViewHalfHeight);

    shader_.use();
    shader_.setMat4("uProjection", projection);
    mesh_.draw();
}

void Lensing2D::rebuild() {
    // Kerr traces backwards in time, in the hole with the spin reversed.
    // These rays run forwards, so that hole is the one with -spin.
    const Kerr hole(rs_, -spin_);
    TraceSettings settings;
    settings.escapeRadius = kEscapeRadius;

    mesh_.clear();
    for (int i = 0; i < kRayCount; ++i) {
        // Moving +x at height b: L = -b about +y (clockwise on screen)
        const double b = beamOffset_ + (i - kRayCount / 2) * kRaySpacing;
        const TracedRay3 ray = hole.trace(
            hole.incomingPhoton(toWorld(glm::dvec2(kBeamStartX, b)), -b),
            settings);
        std::vector<glm::vec2> path;
        path.reserve(ray.path.size());
        for (const glm::dvec3& p : ray.path) {
            path.push_back(toScreen(p));
        }
        mesh_.add(GL_LINE_STRIP, path, fateColor(ray.fate));
    }

    // Boyer-Lindquist r in the equator lies at distance sqrt(r^2 + a^2)
    const auto ring = [&](double r) {
        return static_cast<float>(std::sqrt(r * r + hole.a() * hole.a()));
    };
    // After rays: horizon hides their last step
    const float horizon = ring(hole.horizonRadius());
    mesh_.add(GL_TRIANGLE_FAN, disk(horizon), kHorizonColor);
    mesh_.add(GL_LINE_LOOP, circle(horizon), kHorizonEdgeColor);
    if (spin_ != 0.0) { // else it is the horizon
        mesh_.add(GL_LINE_LOOP, circle(ring(2.0 * hole.mass())),
                  kErgosphereColor);
    }
    // Prograde for this hole is retrograde for the -spin one
    mesh_.add(GL_LINE_LOOP, circle(ring(hole.photonOrbitRadius(false))),
              kProgradeOrbitColor);
    mesh_.add(GL_LINE_LOOP, circle(ring(hole.photonOrbitRadius(true))),
              kRetrogradeOrbitColor);
    mesh_.upload();
}
