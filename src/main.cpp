// Black Hole Simulator
//
// Tests GLFW opens a window, GLEW loads OpenGL, and everything links. Currently
// only opens a dark blue window.

#include <GL/glew.h> // OpenGL loader; must be included before GLFW
#include <GLFW/glfw3.h>

#include <cstdio>

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "Failed to initialize GLFW\n");
        return 1;
    }

    // Ask for a modern OpenGL 4.3 core context (4.3 = compute shaders later).
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window =
        glfwCreateWindow(960, 540, "Black Hole Simulator", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "Failed to create window\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    // Load OpenGL function pointers via GLEW.
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::fprintf(stderr, "Failed to initialize GLEW\n");
        glfwTerminate();
        return 1;
    }

    std::printf("OpenGL %s\n", glGetString(GL_VERSION));
    std::printf("Renderer: %s\n", glGetString(GL_RENDERER));

    // Render loop: clear to a dark blue and present, until ESC or close.
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.02f, 0.02f, 0.06f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(window);
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}