#include "render/Bloom.hpp"

namespace {

constexpr int kLevels = 6;         // 960x540 -> 15x8 at the bottom
constexpr float kThreshold = 1.0f; // HDR brightness where glow starts
constexpr float kKnee = 0.5f;      // soft ramp below the threshold

void dispatch(const Shader& shader, glm::ivec2 size) {
    shader.dispatch(size);
    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT |
                    GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

} // namespace

Bloom::Bloom()
    : down_(BHS_ASSET_DIR "/shaders/bloom_down.comp"),
      up_(BHS_ASSET_DIR "/shaders/bloom_up.comp") {
    for (int i = 0; i < kLevels; ++i) {
        levels_.push_back(std::make_unique<Texture2D>(GL_RGBA16F));
    }
}

const Texture2D& Bloom::apply(const Texture2D& hdr) {
    glm::ivec2 size = hdr.size();
    for (auto& level : levels_) {
        size = glm::max(size / 2, 1);
        level->resize(size);
    }

    // Bright-pass into level 0, then blur downwards
    down_.use();
    down_.setInt("uSource", 0);
    down_.setFloat("uThreshold", kThreshold);
    down_.setFloat("uKnee", kKnee);
    const Texture2D* source = &hdr;
    for (auto& level : levels_) {
        down_.setInt("uBrightPass", source == &hdr);
        source->bind(0);
        level->bindImage(0, GL_WRITE_ONLY);
        dispatch(down_, level->size());
        source = level.get();
    }

    // Each level += blurred smaller level, back up to level 0
    up_.use();
    up_.setInt("uSource", 0);
    for (int i = kLevels - 2; i >= 0; --i) {
        levels_[i + 1]->bind(0);
        levels_[i]->bindImage(0, GL_READ_WRITE);
        dispatch(up_, levels_[i]->size());
    }
    return *levels_[0];
}
