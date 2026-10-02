#include "core/Shader.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <initializer_list>
#include <sstream>
#include <stdexcept>

namespace {

std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Could not open shader file: " + path);
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        glDeleteShader(shader);
        throw std::runtime_error(std::string("Shader compile error:\n") + log);
    }
    return shader;
}

// Links and deletes the stages
GLuint linkProgram(std::initializer_list<GLuint> stages) {
    GLuint program = glCreateProgram();
    for (GLuint stage : stages) {
        glAttachShader(program, stage);
    }
    glLinkProgram(program);
    for (GLuint stage : stages) {
        glDeleteShader(stage);
    }

    GLint linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        char log[1024];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        glDeleteProgram(program);
        throw std::runtime_error(std::string("Program link error:\n") + log);
    }
    return program;
}

} // namespace

Shader::Shader(const std::string& vertPath, const std::string& fragPath) {
    const std::string vertCode = readFile(vertPath);
    const std::string fragCode = readFile(fragPath);

    GLuint vert = compileShader(GL_VERTEX_SHADER, vertCode.c_str());
    GLuint frag = 0;
    try {
        frag = compileShader(GL_FRAGMENT_SHADER, fragCode.c_str());
    } catch (...) {
        glDeleteShader(vert);
        throw;
    }
    program_ = linkProgram({vert, frag});
}

Shader::Shader(const std::string& computePath) {
    const std::string code = readFile(computePath);
    program_ = linkProgram({compileShader(GL_COMPUTE_SHADER, code.c_str())});
}

Shader::~Shader() {
    glDeleteProgram(program_);
}

void Shader::use() const {
    glUseProgram(program_);
}

void Shader::setInt(const std::string& name, int value) const {
    glUniform1i(glGetUniformLocation(program_, name.c_str()), value);
}

void Shader::setFloat(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(program_, name.c_str()), value);
}

void Shader::setVec3(const std::string& name, const glm::vec3& value) const {
    glUniform3fv(glGetUniformLocation(program_, name.c_str()), 1,
                 glm::value_ptr(value));
}

void Shader::setVec4(const std::string& name, const glm::vec4& value) const {
    glUniform4fv(glGetUniformLocation(program_, name.c_str()), 1,
                 glm::value_ptr(value));
}

void Shader::setMat4(const std::string& name, const glm::mat4& value) const {
    GLint location = glGetUniformLocation(program_, name.c_str());
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::dispatch(glm::ivec2 size) const {
    GLint local[3] = {1, 1, 1}; // layout(local_size_*) of the program
    glGetProgramiv(program_, GL_COMPUTE_WORK_GROUP_SIZE, local);
    glDispatchCompute((size.x + local[0] - 1) / local[0],
                      (size.y + local[1] - 1) / local[1], 1);
}
