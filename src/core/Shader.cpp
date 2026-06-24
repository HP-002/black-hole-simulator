#include "core/Shader.hpp"

#include <cstdio>
#include <fstream>
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
        std::fprintf(stderr, "Shader compile error:\n%s\n", log);
    }
    return shader;
}

} // namespace

Shader::Shader(const std::string& vertPath, const std::string& fragPath) {
    const std::string vertCode = readFile(vertPath);
    const std::string fragCode = readFile(fragPath);

    GLuint vert = compileShader(GL_VERTEX_SHADER, vertCode.c_str());
    GLuint frag = compileShader(GL_FRAGMENT_SHADER, fragCode.c_str());

    program_ = glCreateProgram();
    glAttachShader(program_, vert);
    glAttachShader(program_, frag);
    glLinkProgram(program_);

    GLint linked = 0;
    glGetProgramiv(program_, GL_LINK_STATUS, &linked);
    if (!linked) {
        char log[1024];
        glGetProgramInfoLog(program_, sizeof(log), nullptr, log);
        std::fprintf(stderr, "Program link error:\n%s\n", log);
    }

    glDeleteShader(vert);
    glDeleteShader(frag);
}

Shader::~Shader() {
    glDeleteProgram(program_);
}

void Shader::use() const {
    glUseProgram(program_);
}
