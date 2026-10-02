#include "app/BlackHole3D.hpp"

#include "physics/AccretionDisk.hpp"
#include "physics/Kerr.hpp"
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
constexpr float kDiskOuter = 12.0f;       // in rs; inner edge is the ISCO
constexpr float kStartSpin = 0.6f;        // a / M
constexpr float kMaxSpin = 0.998f;        // Thorne's limit for real holes
constexpr float kSpinRate = 0.4f;         // a / M per second
constexpr float kLowRenderScale = 0.5f;
constexpr float kMinExposure = 1.0f / 16.0f;
constexpr float kMaxExposure = 16.0f;
constexpr float kExposureRate = 1.0f; // log(exposure) per second
constexpr float kBloomStrength = 0.1f; // bloom sums 6 levels
constexpr float kTimeScale = 5.0f;     // rs / c per second; inner orbit ~9 s
constexpr double kTimeChunk = 1024.0;  // rs / c; see uTimeChunks in tracer.comp
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
              -glm::degrees(std::atan2(kStartPosition.y, kStartPosition.z))),
      spin_(kStartSpin) {
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

    if (window.isKeyPressed(GLFW_KEY_RIGHT_BRACKET)) {
        spin_ = std::fmin(spin_ + kSpinRate * deltaTime, kMaxSpin);
    }
    if (window.isKeyPressed(GLFW_KEY_LEFT_BRACKET)) {
        spin_ = std::fmax(spin_ - kSpinRate * deltaTime, -kMaxSpin);
    }
    if (window.wasKeyPressed(GLFW_KEY_0)) {
        spin_ = 0.0f;
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

    // Physics in double, once per frame; the shader gets floats
    const Kerr hole(rs_, spin_);
    const AccretionDisk disk(rs_, spin_, kDiskOuter * rs_);
    const glm::dvec3 position(camera_.position());
    const CameraFrame frame =
        hole.cameraFrame(position, glm::dvec3(camera_.right()),
                         glm::dvec3(camera_.up()), glm::dvec3(camera_.front()));

    tracer_.use();
    tracer_.setVec3("uCameraPos", camera_.position());
    tracer_.setVec4("uFrameTime", glm::vec4(frame.time));
    tracer_.setVec4("uFrameRight", glm::vec4(frame.right));
    tracer_.setVec4("uFrameUp", glm::vec4(frame.up));
    tracer_.setVec4("uFrameForward", glm::vec4(frame.forward));
    tracer_.setInt("uCameraOutside", frame.valid);
    tracer_.setFloat("uTanHalfFov", std::tan(glm::radians(kFovDegrees) / 2.0f));
    // Screen aspect: the rounded image is stretched over the whole screen
    tracer_.setFloat("uAspect",
                     static_cast<float>(framebufferSize.x) / framebufferSize.y);
    tracer_.setFloat("uMass", static_cast<float>(hole.mass()));
    tracer_.setFloat("uSpin", spin_);
    tracer_.setFloat("uHorizon", static_cast<float>(hole.horizonRadius()));
    tracer_.setFloat("uEscapeRadius",
                     std::fmax(kMinEscapeRadius * rs_,
                               2.0f * glm::length(camera_.position())));
    tracer_.setFloat("uDiskInner", static_cast<float>(disk.innerRadius()));
    tracer_.setFloat("uDiskOuter", static_cast<float>(disk.outerRadius()));
    tracer_.setFloat("uFluxX0", static_cast<float>(disk.fluxX0()));
    tracer_.setVec3("uFluxRoots", glm::vec3(disk.fluxRoots()));
    tracer_.setVec3("uFluxCoefficients", glm::vec3(disk.fluxCoefficients()));
    tracer_.setFloat("uFluxScale", static_cast<float>(disk.fluxScale()));
    tracer_.setInt("uSkybox", 0);
    const double chunks = std::floor(time_ / kTimeChunk) * kTimeChunk;
    tracer_.setFloat("uTimeChunks", static_cast<float>(chunks));
    tracer_.setFloat("uTimeRest", static_cast<float>(time_ - chunks));

    skybox_.bind(0);
    image_.bindImage(0, GL_WRITE_ONLY);
    tracer_.dispatch(size);
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
