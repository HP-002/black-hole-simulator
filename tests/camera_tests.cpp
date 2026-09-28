// Unit tests for core/Camera

#include "core/Camera.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <cstdio>

namespace {

int failures = 0;

void check(bool condition, const char* what) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", what);
        ++failures;
    }
}

bool approxEqual(float a, float b, float eps = 1e-5f) {
    return std::abs(a - b) <= eps;
}

bool approxEqual(const glm::vec3& a, const glm::vec3& b, float eps = 1e-5f) {
    return approxEqual(a.x, b.x, eps) && approxEqual(a.y, b.y, eps) &&
           approxEqual(a.z, b.z, eps);
}

bool approxEqual(const glm::mat4& a, const glm::mat4& b, float eps = 1e-5f) {
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            if (!approxEqual(a[col][row], b[col][row], eps)) {
                return false;
            }
        }
    }
    return true;
}

bool isFinite(const glm::vec3& v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

void defaultViewMatchesLookAt() {
    const Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
    const glm::mat4 expected =
        glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 1.0f, 0.0f));
    check(approxEqual(camera.viewMatrix(), expected),
          "default camera at (0,0,3) matches lookAt toward the origin");
}

void forwardThenBackwardReturnsHome() {
    Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
    camera.move(Camera::Direction::Forward, 1.0f);
    check(approxEqual(camera.position(),
                      glm::vec3(0.0f, 0.0f, 3.0f - Camera::kMoveSpeed)),
          "1s forward moves kMoveSpeed units toward -z");
    camera.move(Camera::Direction::Backward, 1.0f);
    check(approxEqual(camera.position(), glm::vec3(0.0f, 0.0f, 3.0f)),
          "1s backward undoes 1s forward");
}

void strafeAndVertical() {
    Camera camera(glm::vec3(0.0f));
    camera.move(Camera::Direction::Right, 1.0f);
    check(approxEqual(camera.position(),
                      glm::vec3(Camera::kMoveSpeed, 0.0f, 0.0f)),
          "strafing right moves toward +x");
    camera.move(Camera::Direction::Up, 1.0f);
    check(approxEqual(camera.position().y, Camera::kMoveSpeed),
          "moving up moves toward +y");

    camera.move(Camera::Direction::Left, 1.0f);
    check(approxEqual(camera.position().x, 0.0f),
          "strafing left undoes strafing right");
    camera.move(Camera::Direction::Down, 1.0f);
    check(approxEqual(camera.position().y, 0.0f),
          "moving down undoes moving up");
}

void yawTurnsRight() {
    Camera camera(glm::vec3(0.0f));
    camera.rotate(90.0f, 0.0f);
    check(approxEqual(camera.front(), glm::vec3(1.0f, 0.0f, 0.0f)),
          "+90 degrees yaw turns from -z to +x");
}

void pitchIsClampedAndStaysFinite() {
    Camera camera(glm::vec3(0.0f));

    camera.rotate(0.0f, 1000.0f);
    check(approxEqual(camera.pitch(), Camera::kMaxPitch),
          "pitch clamps at +kMaxPitch");
    check(isFinite(camera.front()) && isFinite(camera.up()),
          "basis stays finite looking straight up");

    camera.rotate(0.0f, -2000.0f);
    check(approxEqual(camera.pitch(), -Camera::kMaxPitch),
          "pitch clamps at -kMaxPitch");
    check(isFinite(camera.front()) && isFinite(camera.up()),
          "basis stays finite looking straight down");
}

} // namespace

int main() {
    defaultViewMatchesLookAt();
    forwardThenBackwardReturnsHome();
    strafeAndVertical();
    yawTurnsRight();
    pitchIsClampedAndStaysFinite();

    if (failures == 0) {
        std::printf("All camera tests passed\n");
    }
    return failures == 0 ? 0 : 1;
}
