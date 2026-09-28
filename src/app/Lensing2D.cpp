#include "app/Lensing2D.hpp"

#include "physics/Schwarzschild.hpp"

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
constexpr int kCircleSegments = 128;

const glm::vec3 kEscapedColor(1.0f, 0.8f, 0.3f);
const glm::vec3 kCapturedColor(0.9f, 0.25f, 0.2f);
const glm::vec3 kUnfinishedColor(1.0f, 1.0f, 1.0f);
const glm::vec3 kHorizonColor(0.0f, 0.0f, 0.0f);
const glm::vec3 kHorizonEdgeColor(1.0f, 0.5f, 0.1f);
const glm::vec3 kPhotonSphereColor(0.4f, 0.5f, 0.8f);

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

} // namespace

Lensing2D::Lensing2D()
    : shader_(BHS_ASSET_DIR "/shaders/mesh2d.vert",
              BHS_ASSET_DIR "/shaders/mesh2d.frag") {}

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
    const Schwarzschild hole(rs_);
    TraceSettings settings;
    settings.escapeRadius = kEscapeRadius;

    mesh_.clear();
    for (int i = 0; i < kRayCount; ++i) {
        const double b = beamOffset_ + (i - kRayCount / 2) * kRaySpacing;
        const TracedRay ray = hole.trace(glm::dvec2(kBeamStartX, b),
                                         glm::dvec2(1.0, 0.0), settings);
        const std::vector<glm::vec2> path(ray.path.begin(), ray.path.end());
        mesh_.add(GL_LINE_STRIP, path, fateColor(ray.fate));
    }

    // After rays: horizon hides their last step
    const float rs = static_cast<float>(rs_);
    mesh_.add(GL_TRIANGLE_FAN, disk(rs), kHorizonColor);
    mesh_.add(GL_LINE_LOOP, circle(rs), kHorizonEdgeColor);
    mesh_.add(GL_LINE_LOOP,
              circle(static_cast<float>(hole.photonSphereRadius())),
              kPhotonSphereColor);
    mesh_.upload();
}
