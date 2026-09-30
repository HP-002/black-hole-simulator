// Unit tests for core/Png

#include "core/Png.hpp"

#include <cstdio>
#include <stdexcept>

namespace {

int failures = 0;

void check(bool condition, const char* what) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", what);
        ++failures;
    }
}

using Bytes = std::vector<std::uint8_t>;

bool matchesAt(const Bytes& data, std::size_t pos, const Bytes& expected) {
    if (pos + expected.size() > data.size()) {
        return false;
    }
    for (std::size_t i = 0; i < expected.size(); ++i) {
        if (data[pos + i] != expected[i]) {
            return false;
        }
    }
    return true;
}

void singlePixel() {
    const Bytes png = encodePng(1, 1, {0, 0, 0});
    check(matchesAt(png, 0, {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'}),
          "PNG signature");
    // Length 13, "IHDR", 1 x 1, 8-bit RGB, CRC
    check(matchesAt(png, 8,
                    {0, 0, 0, 13, 'I', 'H', 'D', 'R', 0, 0, 0, 1, 0, 0, 0, 1,
                     8, 2, 0, 0, 0, 0x90, 0x77, 0x53, 0xDE}),
          "IHDR chunk with known CRC");
    // Length 15: zlib header, stored block of 4 bytes, Adler-32
    check(matchesAt(png, 33,
                    {0, 0, 0, 15, 'I', 'D', 'A', 'T', 0x78, 0x01, 0x01, 4, 0,
                     0xFB, 0xFF, 0, 0, 0, 0, 0x00, 0x04, 0x00, 0x01}),
          "IDAT holds one stored block and its Adler-32");
    check(matchesAt(png, png.size() - 12,
                    {0, 0, 0, 0, 'I', 'E', 'N', 'D', 0xAE, 0x42, 0x60, 0x82}),
          "IEND chunk with known CRC");
}

void splitsLargeImages() {
    // 90001 raw bytes -> two stored blocks
    const int width = 30000;
    const Bytes png = encodePng(width, 1, Bytes(width * 3, 7));
    const std::size_t idat = 12 + 2 + 2 * 5 + 90001 + 4;
    check(png.size() == 8 + 25 + idat + 12, "two stored blocks");
    check(png[33 + 8 + 2] == 0 && png[33 + 8 + 2 + 5 + 65535] == 1,
          "only the last block is final");
}

void rejectsBadSize() {
    bool threw = false;
    try {
        encodePng(2, 2, Bytes(5));
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    check(threw, "pixel count must match the size");
}

} // namespace

int main() {
    singlePixel();
    splitsLargeImages();
    rejectsBadSize();

    if (failures == 0) {
        std::printf("All PNG tests passed\n");
    }
    return failures == 0 ? 0 : 1;
}
