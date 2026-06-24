// Black Hole Simulator
//
// Entry point. Opens a window, loads the triangle shader, and runs the render
// loop. Windowing and shader plumbing live in core/; main() just wires them up.

#include "core/Shader.hpp"
#include "core/Window.hpp"

#include <cstdio>
#include <stdexcept>

int main() {
    try {
        Window window(960, 540, "Black Hole Simulator");

        // Triangle in Normalized Device Coordinates (NDC), at the screen
        // center. Each vertex is position (x, y, z) followed by color (r, g,
        // b).
        float vertices[] = {-0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
                            0.5f,  -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
                            0.0f,  0.5f,  0.0f, 0.0f, 0.0f, 1.0f};

        GLuint vao, vbo;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices,
                     GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                              (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                              (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);

        // BHS_ASSET_DIR is the absolute path to assets/, baked in at compile
        // time by CMake (see CMakeLists.txt) so the exe finds shaders from any
        // cwd.
        Shader shader(BHS_ASSET_DIR "/shaders/triangle.vert",
                      BHS_ASSET_DIR "/shaders/triangle.frag");

        // Render loop: clear to dark blue, draw the triangle, until ESC or
        // close.
        while (!window.shouldClose()) {
            glClearColor(0.02f, 0.02f, 0.06f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            shader.use();
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, 3);

            window.swapBuffers();
            window.pollEvents();

            if (window.isKeyPressed(GLFW_KEY_ESCAPE)) {
                window.requestClose();
            }
        }

        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Fatal: %s\n", e.what());
        return 1;
    }
    return 0;
}
