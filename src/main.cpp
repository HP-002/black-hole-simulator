// Black Hole Simulator

// Entry point

#include "app/BlackHole3D.hpp"
#include "app/Lensing2D.hpp"
#include "core/Window.hpp"

#include <cstdio>
#include <stdexcept>

int main() {
    try {
        Window window(960, 540, "Black Hole Simulator");
        BlackHole3D view3D;
        Lensing2D view2D;
        bool show3D = true;

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
            if (window.isKeyPressed(GLFW_KEY_1)) {
                show3D = true;
            }
            if (window.isKeyPressed(GLFW_KEY_2)) {
                show3D = false;
            }

            glClearColor(0.02f, 0.02f, 0.06f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            if (show3D) {
                view3D.update(window, deltaTime);
                view3D.render(window.aspectRatio());
            } else {
                view2D.update(window, deltaTime);
                view2D.render(window.aspectRatio());
            }

            window.swapBuffers();
        }
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Fatal: %s\n", e.what());
        return 1;
    }
    return 0;
}
