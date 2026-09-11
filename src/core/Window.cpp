#include "core/Window.hpp"

#include <cstdio>
#include <stdexcept>

Window::Window(int width, int height, const char* title) {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    // Ask for a modern OpenGL 4.3 core context (4.3 = compute shaders later).
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window_) {
        glfwTerminate();
        throw std::runtime_error("Failed to create window");
    }
    glfwMakeContextCurrent(window_);

    // Keep the GL viewport matched to the framebuffer when the window resizes.
    glfwSetFramebufferSizeCallback(
        window_, [](GLFWwindow*, int w, int h) { glViewport(0, 0, w, h); });

    // Hide the cursor and lock it to the window
    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(window_, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    // Load OpenGL function pointers via GLEW (requires a current context
    // first).
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        glfwDestroyWindow(window_);
        glfwTerminate();
        throw std::runtime_error("Failed to initialize GLEW");
    }

    std::printf("OpenGL %s\n", glGetString(GL_VERSION));
    std::printf("Renderer: %s\n", glGetString(GL_RENDERER));
}

Window::~Window() {
    if (window_) {
        glfwDestroyWindow(window_);
    }
    glfwTerminate();
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(window_);
}

void Window::requestClose() {
    glfwSetWindowShouldClose(window_, GLFW_TRUE);
}

void Window::swapBuffers() {
    glfwSwapBuffers(window_);
}

void Window::pollEvents() {
    glfwPollEvents();

    if (!glfwGetWindowAttrib(window_, GLFW_FOCUSED)) {
        hasLastCursorPos_ = false;
        cursorDelta_ = glm::vec2(0.0f);
        return;
    }

    double x = 0.0;
    double y = 0.0;
    glfwGetCursorPos(window_, &x, &y);
    const glm::dvec2 cursorPos(x, y);

    if (!hasLastCursorPos_) {
        lastCursorPos_ = cursorPos;
        hasLastCursorPos_ = true;
    }
    cursorDelta_ = glm::vec2(cursorPos - lastCursorPos_);
    lastCursorPos_ = cursorPos;
}

bool Window::isKeyPressed(int key) const {
    return glfwGetKey(window_, key) == GLFW_PRESS;
}

float Window::aspectRatio() const {
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window_, &width, &height);
    // Check for division by zero
    return height > 0 ? static_cast<float>(width) / height : 1.0f;
}
