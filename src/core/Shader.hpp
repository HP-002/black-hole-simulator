#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <string>

class Shader {
  public:
    Shader(const std::string& vertPath, const std::string& fragPath);
    ~Shader();

    // Non-copyable: it uniquely owns a GL program object.
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    void use() const;
    GLuint id() const { return program_; }

    void setMat4(const std::string& name, const glm::mat4& value) const;

  private:
    GLuint program_ = 0;
};
