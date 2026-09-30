#include "app/BlackHole3D.hpp"

#include "render/StarField.hpp"

#include <cmath>

namespace {

constexpr int kSkyboxSize = 1024; // texels per face
constexpr int kStarCount = 12000;
constexpr unsigned kStarSeed = 7;
constexpr float kFovDegrees = 60.0f;      // vertical
constexpr float kMouseSensitivity = 0.1f; // degrees per pixel
constexpr float kMinRs = 0.25f;
constexpr float kMaxRs = 3.0f;
constexpr float kMassRate = 0.5f;         // log(rs) per second
constexpr float kMinEscapeRadius = 50.0f; // in rs; bending left < 1e-4 rad
constexpr float kDiskInner = 3.0f;        // in rs; innermost stable orbit
constexpr float kDiskOuter = 12.0f;       // in rs
constexpr int kWorkGroupSize = 8;         // tracer.comp local size
constexpr float kLowRenderScale = 0.5f;
constexpr float kMinExposure = 1.0f / 16.0f;
constexpr float kMaxExposure = 16.0f;
constexpr float kExposureRate = 1.0f; // log(exposure) per second
constexpr float kBloomStrength = 0.1f; // bloom sums 6 levels
constexpr float kTimeScale = 5.0f;     // rs / c per second; inner orbit ~9 s
const glm::vec3 kStartPosition(0.0f, 2.0f, 20.0f);

} // namespace

BlackHole3D::BlackHole3D()
    : tracer_(BHS_ASSET_DIR "/shaders/tracer.comp"),
      toneMap_(BHS_ASSET_DIR "/shaders/fullscreen.vert",
               BHS_ASSET_DIR "/shaders/tonemap.frag"),
      image_(GL_RGBA16F),
      skybox_(generateStarField(kSkyboxSize, kStarCount, kStarSeed)),
      // Pitched to look at the origin
      camera_(kStartPosition, -90.0f,
              -glm::degrees(std::atan2(kStartPosition.y, kStartPosition.z))) {
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

void BlackHole3D::update(const Window& window, float deltaTime) {
    if (window.wasKeyPressed(GLFW_KEY_P)) {
        paused_ = !paused_;
    }
    // In rs / c: changing the mass doesn't jolt the disk
    if (!paused_) {
        time_ += kTimeScale * deltaTime;
    }

    if (window.isKeyPressed(GLFW_KEY_W)) {
        camera_.move(Camera::Direction::Forward, deltaTime);
    }
    if (window.isKeyPressed(GLFW_KEY_S)) {
        camera_.move(Camera::Direction::Backward, deltaTime);
    }
    if (window.isKeyPressed(GLFW_KEY_A)) {
        camera_.move(Camera::Direction::Left, deltaTime);
    }
    if (window.isKeyPressed(GLFW_KEY_D)) {
        camera_.move(Camera::Direction::Right, deltaTime);
    }
    if (window.isKeyPressed(GLFW_KEY_SPACE)) {
        camera_.move(Camera::Direction::Up, deltaTime);
    }
    if (window.isKeyPressed(GLFW_KEY_LEFT_SHIFT)) {
        camera_.move(Camera::Direction::Down, deltaTime);
    }

    if (window.wasKeyPressed(GLFW_KEY_R)) {
        renderScale_ = renderScale_ == 1.0f ? kLowRenderScale : 1.0f;
    }

    const glm::vec2 mouse = window.cursorDelta();
    camera_.rotate(mouse.x * kMouseSensitivity, -mouse.y * kMouseSensitivity);

    if (window.isKeyPressed(GLFW_KEY_UP)) {
        rs_ = std::fmin(rs_ * std::exp(kMassRate * deltaTime), kMaxRs);
    }
    if (window.isKeyPressed(GLFW_KEY_DOWN)) {
        rs_ = std::fmax(rs_ * std::exp(-kMassRate * deltaTime), kMinRs);
    }

    if (window.isKeyPressed(GLFW_KEY_E)) {
        exposure_ = std::fmin(exposure_ * std::exp(kExposureRate * deltaTime),
                              kMaxExposure);
    }
    if (window.isKeyPressed(GLFW_KEY_Q)) {
        exposure_ = std::fmax(exposure_ * std::exp(-kExposureRate * deltaTime),
                              kMinExposure);
    }
}

void BlackHole3D::render(glm::ivec2 framebufferSize) {
    if (framebufferSize.x <= 0 || framebufferSize.y <= 0) {
        return; // minimized
    }
    image_.resize(glm::max(
        glm::ivec2(glm::vec2(framebufferSize) * renderScale_ + 0.5f), 1));
    const glm::ivec2 size = image_.size();

    tracer_.use();
    tracer_.setVec3("uCameraPos", camera_.position());
    tracer_.setVec3("uCameraFront", camera_.front());
    tracer_.setVec3("uCameraRight", camera_.right());
    tracer_.setVec3("uCameraUp", camera_.up());
    tracer_.setFloat("uTanHalfFov", std::tan(glm::radians(kFovDegrees) / 2.0f));
    tracer_.setFloat("uAspect", static_cast<float>(size.x) / size.y);
    tracer_.setFloat("uRs", rs_);
    tracer_.setFloat("uEscapeRadius",
                     std::fmax(kMinEscapeRadius * rs_,
                               2.0f * glm::length(camera_.position())));
    tracer_.setFloat("uDiskInner", kDiskInner * rs_);
    tracer_.setFloat("uDiskOuter", kDiskOuter * rs_);
    tracer_.setInt("uSkybox", 0);
    tracer_.setFloat("uTime", time_);

    skybox_.bind(0);
    image_.bindImage(0, GL_WRITE_ONLY);
    glDispatchCompute((size.x + kWorkGroupSize - 1) / kWorkGroupSize,
                      (size.y + kWorkGroupSize - 1) / kWorkGroupSize, 1);
    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

    const Texture2D& bloom = bloom_.apply(image_);

    toneMap_.use();
    toneMap_.setInt("uImage", 0);
    toneMap_.setInt("uBloom", 1);
    toneMap_.setFloat("uBloomStrength", kBloomStrength);
    toneMap_.setFloat("uExposure", exposure_);
    image_.bind(0);
    bloom.bind(1);
    screen_.draw();
}
