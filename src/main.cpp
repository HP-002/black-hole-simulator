// Black Hole Simulator

// Entry point

#include "app/Lensing2D.hpp"
#include "core/Window.hpp"

#include <cstdio>
#include <stdexcept>

int main() {
    try {
        Window window(960, 540, "Black Hole Simulator");
        Lensing2D demo;

        double lastFrameTime = glfwGetTime();

        // Render loop
        while (!window.shouldClose()) {
            window.pollEvents();

            const double now = glfwGetTime();
            const float deltaTime = static_cast<float>(now - lastFrameTime);
            lastFrameTime = now;

            if (window.isKeyPressed(GLFW_KEY_ESCAPE)) {
                window.requestClose();
            }
            demo.update(window, deltaTime);

            glClearColor(0.02f, 0.02f, 0.06f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            demo.render(window.aspectRatio());

            window.swapBuffers();
        }
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Fatal: %s\n", e.what());
        return 1;
    }
    return 0;
}
