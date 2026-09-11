#include "core/Camera.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

namespace {

const glm::vec3 kWorldUp(0.0f, 1.0f, 0.0f);

}

Camera::Camera(const glm::vec3& position, float yaw, float pitch)
    : position_(position), yaw_(yaw),
      pitch_(glm::clamp(pitch, -kMaxPitch, kMaxPitch)) {
    updateVectors();
}

glm::mat4 Camera::viewMatrix() const {
    return glm::lookAt(position_, position_ + front_, up_);
}

void Camera::move(Direction direction, float deltaTime) {
    const float distance = kMoveSpeed * deltaTime;
    switch (direction) {
    case Direction::Forward:
        position_ += front_ * distance;
        break;
    case Direction::Backward:
        position_ -= front_ * distance;
        break;
    case Direction::Left:
        position_ -= right_ * distance;
        break;
    case Direction::Right:
        position_ += right_ * distance;
        break;
    case Direction::Up:
        position_ += kWorldUp * distance;
        break;
    case Direction::Down:
        position_ -= kWorldUp * distance;
        break;
    }
}

void Camera::rotate(float yawDegrees, float pitchDegrees) {
    yaw_ += yawDegrees;
    pitch_ = glm::clamp(pitch_ + pitchDegrees, -kMaxPitch, kMaxPitch);
    updateVectors();
}

void Camera::updateVectors() {
    // Spherical to Cartesian
    const float yaw = glm::radians(yaw_);
    const float pitch = glm::radians(pitch_);
    front_ = glm::normalize(glm::vec3(std::cos(yaw) * std::cos(pitch),
                                      std::sin(pitch),
                                      std::sin(yaw) * std::cos(pitch)));
    right_ = glm::normalize(glm::cross(front_, kWorldUp));
    up_ = glm::cross(right_, front_);
}
