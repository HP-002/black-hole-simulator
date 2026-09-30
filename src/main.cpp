// Black Hole Simulator

// Entry point

#include "app/BlackHole3D.hpp"
#include "app/Lensing2D.hpp"
#include "core/Png.hpp"
#include "core/Window.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>

namespace {

constexpr double kFpsInterval = 0.5; // seconds between title updates
constexpr int kTitleSize = 128;

// Offscreen color target for --screenshot. The window's own pixels are
// undefined where it is covered or offscreen.
class OffscreenTarget {
  public:
    explicit OffscreenTarget(glm::ivec2 size) : size_(size) {
        glGenRenderbuffers(1, &color_);
        glBindRenderbuffer(GL_RENDERBUFFER, color_);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, size.x, size.y);
        glGenFramebuffers(1, &fbo_);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                  GL_RENDERBUFFER, color_);
        const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            release();
            throw std::runtime_error("Screenshot framebuffer incomplete");
        }
    }
    ~OffscreenTarget() { release(); }

    OffscreenTarget(const OffscreenTarget&) = delete;
    OffscreenTarget& operator=(const OffscreenTarget&) = delete;

    void bind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
        glViewport(0, 0, size_.x, size_.y);
    }
    glm::ivec2 size() const { return size_; }

  private:
    void release() {
        glDeleteFramebuffers(1, &fbo_);
        glDeleteRenderbuffers(1, &color_);
    }

    glm::ivec2 size_;
    GLuint fbo_ = 0;
    GLuint color_ = 0;
};

// Bound target -> PNG, flipped to top-down rows
void saveScreenshot(const OffscreenTarget& target, const std::string& path) {
    const glm::ivec2 size = target.size();
    const std::size_t rowBytes = static_cast<std::size_t>(size.x) * 3;
    std::vector<std::uint8_t> pixels(rowBytes * size.y);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
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
        std::unique_ptr<OffscreenTarget> screenshotTarget;
        if (!screenshotPath.empty()) {
            screenshotTarget =
                std::make_unique<OffscreenTarget>(window.framebufferSize());
            screenshotTarget->bind();
        }

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
                char title[kTitleSize];
                int used = std::snprintf(
                    title, sizeof(title),
                    "Black Hole Simulator | %.0f FPS (%.1f ms)", fps,
                    1000.0 / fps);
                // Truncated or failed prefix: append nothing past the end
                used = std::clamp(used, 0, kTitleSize - 1);
                if (show3D) {
                    std::snprintf(title + used, sizeof(title) - used,
                                  " | %gx scale | exposure %.2f%s",
                                  view3D.renderScale(), view3D.exposure(),
                                  view3D.paused() ? " | paused" : "");
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
                saveScreenshot(*screenshotTarget, screenshotPath);
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
