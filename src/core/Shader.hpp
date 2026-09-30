#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <string>

class Shader {
  public:
    Shader(const std::string& vertPath, const std::string& fragPath);
    explicit Shader(const std::string& computePath);
    ~Shader();

    // Non-copyable: it uniquely owns a GL program object.
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    void use() const;
    GLuint id() const { return program_; }

    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setMat4(const std::string& name, const glm::mat4& value) const;

    // Compute only: enough work groups to cover size; caller adds barriers
    void dispatch(glm::ivec2 size) const;

  private:
    GLuint program_ = 0;
};
