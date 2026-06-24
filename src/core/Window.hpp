#pragma once

// GLEW MUST be included before GLFW
#include <GL/glew.h>
#include <GLFW/glfw3.h>

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

    GLFWwindow* handle() const { return window_; }

  private:
    GLFWwindow* window_ = nullptr;
};
