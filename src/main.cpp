// Black Hole Simulator

// Entry point

#include "app/BlackHole3D.hpp"
#include "app/Lensing2D.hpp"
#include "core/Png.hpp"
#include "core/Window.hpp"

#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>

namespace {

constexpr double kFpsInterval = 0.5; // seconds between title updates

// Back buffer -> PNG, flipped to top-down rows
void saveScreenshot(const Window& window, const std::string& path) {
    const glm::ivec2 size = window.framebufferSize();
    const std::size_t rowBytes = static_cast<std::size_t>(size.x) * 3;
    std::vector<std::uint8_t> pixels(rowBytes * size.y);
    glReadBuffer(GL_BACK);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, size.x, size.y, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    std::vector<std::uint8_t> flipped(pixels.size());
    for (int y = 0; y < size.y; ++y) {
        std::memcpy(&flipped[rowBytes * y], &pixels[rowBytes * (size.y - 1 - y)],
                    rowBytes);
    }
    writePng(path, size.x, size.y, flipped);
    std::printf("Saved %s (%dx%d)\n", path.c_str(), size.x, size.y);
}

} // namespace

int main(int argc, char** argv) {
    // --screenshot out.png: render one frame, save it, quit
    std::string screenshotPath;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--screenshot") == 0 && i + 1 < argc) {
            screenshotPath = argv[++i];
        } else {
            std::fprintf(stderr, "Usage: %s [--screenshot out.png]\n", argv[0]);
            return 2;
        }
    }

    try {
        Window window(960, 540, "Black Hole Simulator");
        BlackHole3D view3D;
        Lensing2D view2D;
        bool show3D = true;

        double lastFrameTime = glfwGetTime();
        double fpsStart = lastFrameTime;
        int fpsFrames = 0;

        // Render loop
        while (!window.shouldClose()) {
            window.pollEvents();

            const double now = glfwGetTime();
            const float deltaTime = static_cast<float>(now - lastFrameTime);
            lastFrameTime = now;

            // FPS averaged over kFpsInterval
            ++fpsFrames;
            if (now - fpsStart >= kFpsInterval) {
                const double fps = fpsFrames / (now - fpsStart);
                char title[128];
                const int used = std::snprintf(
                    title, sizeof(title),
                    "Black Hole Simulator | %.0f FPS (%.1f ms)", fps,
                    1000.0 / fps);
                if (show3D) {
                    std::snprintf(title + used, sizeof(title) - used,
                                  " | %gx scale", view3D.renderScale());
                }
                window.setTitle(title);
                fpsStart = now;
                fpsFrames = 0;
            }

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
                view3D.render(window.framebufferSize());
            } else {
                view2D.update(window, deltaTime);
                view2D.render(window.aspectRatio());
            }

            if (!screenshotPath.empty()) {
                saveScreenshot(window, screenshotPath);
                break;
            }
            window.swapBuffers();
        }
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Fatal: %s\n", e.what());
        return 1;
    }
    return 0;
}
