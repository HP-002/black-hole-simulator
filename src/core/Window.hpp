#pragma once

// GLEW MUST be included before GLFW
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class Window {
  public:
    Window(int width, int height, const char* title);
    ~Window();

    // Non-copyable: it uniquely owns a GLFW window + GL context.
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool shouldClose() const;
    void requestClose();
    void swapBuffers();
    void pollEvents();
    bool isKeyPressed(int key) const;

    float aspectRatio() const;

    glm::vec2 cursorDelta() const { return cursorDelta_; }

    GLFWwindow* handle() const { return window_; }

  private:
    GLFWwindow* window_ = nullptr;
    glm::dvec2 lastCursorPos_{0.0};
    glm::vec2 cursorDelta_{0.0f};
    bool hasLastCursorPos_ = false;
};
