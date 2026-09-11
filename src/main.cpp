// Black Hole Simulator

// Entry point

#include "core/Shader.hpp"
#include "core/Window.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdio>
#include <stdexcept>

int main() {
    try {
        Window window(960, 540, "Black Hole Simulator");

        // Unit cube centered on the origin.
        // 6 faces x 2 triangles x 3 vertices = 36 vertices
        float vertices[] = {
            // back (z = -0.5), red
            -0.5f, -0.5f, -0.5f,  1.0f, 0.2f, 0.2f,
            -0.5f,  0.5f, -0.5f,  1.0f, 0.2f, 0.2f,
             0.5f,  0.5f, -0.5f,  1.0f, 0.2f, 0.2f,
             0.5f,  0.5f, -0.5f,  1.0f, 0.2f, 0.2f,
             0.5f, -0.5f, -0.5f,  1.0f, 0.2f, 0.2f,
            -0.5f, -0.5f, -0.5f,  1.0f, 0.2f, 0.2f,
            // front (z = +0.5), green
            -0.5f, -0.5f,  0.5f,  0.2f, 1.0f, 0.2f,
             0.5f, -0.5f,  0.5f,  0.2f, 1.0f, 0.2f,
             0.5f,  0.5f,  0.5f,  0.2f, 1.0f, 0.2f,
             0.5f,  0.5f,  0.5f,  0.2f, 1.0f, 0.2f,
            -0.5f,  0.5f,  0.5f,  0.2f, 1.0f, 0.2f,
            -0.5f, -0.5f,  0.5f,  0.2f, 1.0f, 0.2f,
            // left (x = -0.5), blue
            -0.5f,  0.5f,  0.5f,  0.2f, 0.4f, 1.0f,
            -0.5f,  0.5f, -0.5f,  0.2f, 0.4f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.2f, 0.4f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.2f, 0.4f, 1.0f,
            -0.5f, -0.5f,  0.5f,  0.2f, 0.4f, 1.0f,
            -0.5f,  0.5f,  0.5f,  0.2f, 0.4f, 1.0f,
            // right (x = +0.5), yellow
             0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.2f,
             0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 0.2f,
             0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.2f,
             0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.2f,
             0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.2f,
             0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.2f,
            // bottom (y = -0.5), magenta
            -0.5f, -0.5f, -0.5f,  1.0f, 0.2f, 1.0f,
             0.5f, -0.5f, -0.5f,  1.0f, 0.2f, 1.0f,
             0.5f, -0.5f,  0.5f,  1.0f, 0.2f, 1.0f,
             0.5f, -0.5f,  0.5f,  1.0f, 0.2f, 1.0f,
            -0.5f, -0.5f,  0.5f,  1.0f, 0.2f, 1.0f,
            -0.5f, -0.5f, -0.5f,  1.0f, 0.2f, 1.0f,
            // top (y = +0.5), cyan
            -0.5f,  0.5f, -0.5f,  0.2f, 1.0f, 1.0f,
            -0.5f,  0.5f,  0.5f,  0.2f, 1.0f, 1.0f,
             0.5f,  0.5f,  0.5f,  0.2f, 1.0f, 1.0f,
             0.5f,  0.5f,  0.5f,  0.2f, 1.0f, 1.0f,
             0.5f,  0.5f, -0.5f,  0.2f, 1.0f, 1.0f,
            -0.5f,  0.5f, -0.5f,  0.2f, 1.0f, 1.0f,
        };
        // clang-format on
        const GLsizei vertexCount = sizeof(vertices) / (6 * sizeof(float));

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
        Shader shader(BHS_ASSET_DIR "/shaders/cube.vert",
                      BHS_ASSET_DIR "/shaders/cube.frag");

        
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

        const glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f),
                                           glm::vec3(0.0f, 0.0f, 0.0f),
                                           glm::vec3(0.0f, 1.0f, 0.0f));

        // Render loop
        while (!window.shouldClose()) {
            glClearColor(0.02f, 0.02f, 0.06f, 1.0f);
            // Clear depth
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Rotate around a tilted axis
            const float time = static_cast<float>(glfwGetTime());
            const glm::mat4 model = glm::rotate(
                glm::mat4(1.0f), time,
                glm::normalize(glm::vec3(0.5f, 1.0f, 0.0f)));

            const glm::mat4 projection = glm::perspective(
                glm::radians(45.0f), window.aspectRatio(), 0.1f, 100.0f);

            shader.use();
            shader.setMat4("uModel", model);
            shader.setMat4("uView", view);
            shader.setMat4("uProjection", projection);

            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, vertexCount);

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
