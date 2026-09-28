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
constexpr float kMassRate = 0.5f; // log(rs) per second

} // namespace

BlackHole3D::BlackHole3D()
    : shader_(BHS_ASSET_DIR "/shaders/fullscreen.vert",
              BHS_ASSET_DIR "/shaders/blackhole.frag"),
      skybox_(generateStarField(kSkyboxSize, kStarCount, kStarSeed)),
      camera_(glm::vec3(0.0f, 0.0f, 10.0f)) {
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

void BlackHole3D::update(const Window& window, float deltaTime) {
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

    const glm::vec2 mouse = window.cursorDelta();
    camera_.rotate(mouse.x * kMouseSensitivity, -mouse.y * kMouseSensitivity);

    if (window.isKeyPressed(GLFW_KEY_UP)) {
        rs_ = std::fmin(rs_ * std::exp(kMassRate * deltaTime), kMaxRs);
    }
    if (window.isKeyPressed(GLFW_KEY_DOWN)) {
        rs_ = std::fmax(rs_ * std::exp(-kMassRate * deltaTime), kMinRs);
    }
}

void BlackHole3D::render(float aspectRatio) const {
    shader_.use();
    shader_.setVec3("uCameraPos", camera_.position());
    shader_.setVec3("uCameraFront", camera_.front());
    shader_.setVec3("uCameraRight", camera_.right());
    shader_.setVec3("uCameraUp", camera_.up());
    shader_.setFloat("uTanHalfFov", std::tan(glm::radians(kFovDegrees) / 2.0f));
    shader_.setFloat("uAspect", aspectRatio);
    shader_.setFloat("uRs", rs_);
    shader_.setInt("uSkybox", 0);

    skybox_.bind(0);
    screen_.draw();
}
