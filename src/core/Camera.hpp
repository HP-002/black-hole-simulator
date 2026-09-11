#pragma once

#include <glm/glm.hpp>

// Fly Camera
class Camera {
  public:
    enum class Direction { Forward, Backward, Left, Right, Up, Down };

    static constexpr float kMoveSpeed = 2.5f; // world units per second
    static constexpr float kMaxPitch = 89.0f; // degrees

    // yaw = -90 looks down -z
    explicit Camera(const glm::vec3& position, float yaw = -90.0f,
                    float pitch = 0.0f);

    glm::mat4 viewMatrix() const;

    void move(Direction direction, float deltaTime);
    // +yaw turns right, +pitch looks up.
    void rotate(float yawDegrees, float pitchDegrees);

    const glm::vec3& position() const { return position_; }
    const glm::vec3& front() const { return front_; }
    const glm::vec3& up() const { return up_; }
    float pitch() const { return pitch_; }

  private:
    void updateVectors();

    glm::vec3 position_;
    float yaw_;
    float pitch_;
    glm::vec3 front_{0.0f, 0.0f, -1.0f};
    glm::vec3 right_{1.0f, 0.0f, 0.0f};
    glm::vec3 up_{0.0f, 1.0f, 0.0f};
};
