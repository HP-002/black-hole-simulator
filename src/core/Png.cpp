#include "core/Png.hpp"

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <stdexcept>

namespace {

using Bytes = std::vector<std::uint8_t>;

std::uint32_t crc32(const std::uint8_t* data, std::size_t size,
                    std::uint32_t crc = 0) {
    crc = ~crc;
    for (std::size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
        }
    }
    return ~crc;
}

std::uint32_t adler32(const Bytes& data) {
    std::uint32_t a = 1;
    std::uint32_t b = 0;
    for (std::uint8_t byte : data) {
        a = (a + byte) % 65521u;
        b = (b + a) % 65521u;
    }
    return (b << 16) | a;
}

void putBigEndian(Bytes& out, std::uint32_t value) {
    for (int shift = 24; shift >= 0; shift -= 8) {
        out.push_back(static_cast<std::uint8_t>(value >> shift));
    }
}

void putChunk(Bytes& out, const char* type, const Bytes& data) {
    putBigEndian(out, static_cast<std::uint32_t>(data.size()));
    const std::size_t start = out.size();
    out.insert(out.end(), type, type + 4);
    out.insert(out.end(), data.begin(), data.end());
    putBigEndian(out, crc32(out.data() + start, out.size() - start));
}

// zlib stream of stored (uncompressed) deflate blocks
Bytes zlibStore(const Bytes& raw) {
    constexpr std::size_t kMaxBlock = 65535;
    Bytes out = {0x78, 0x01};
    std::size_t pos = 0;
    do {
        const std::size_t len = std::min(kMaxBlock, raw.size() - pos);
        const bool last = pos + len == raw.size();
        out.push_back(last ? 1 : 0);
        out.push_back(static_cast<std::uint8_t>(len));
        out.push_back(static_cast<std::uint8_t>(len >> 8));
        out.push_back(static_cast<std::uint8_t>(~len));
        out.push_back(static_cast<std::uint8_t>(~len >> 8));
        out.insert(out.end(), raw.begin() + pos, raw.begin() + pos + len);
        pos += len;
    } while (pos < raw.size());
    putBigEndian(out, adler32(raw));
    return out;
}

} // namespace

std::vector<std::uint8_t> encodePng(int width, int height,
                                    const std::vector<std::uint8_t>& rgb) {
    const std::size_t rowBytes = static_cast<std::size_t>(width) * 3;
    if (width <= 0 || height <= 0 || rgb.size() != rowBytes * height) {
        throw std::invalid_argument("encodePng: bad image size");
    }

    // Filter type 0 (none) before each row
    Bytes raw;
    raw.reserve((rowBytes + 1) * height);
    for (int y = 0; y < height; ++y) {
        raw.push_back(0);
        const auto row = rgb.begin() + rowBytes * y;
        raw.insert(raw.end(), row, row + rowBytes);
    }

    Bytes header;
    putBigEndian(header, static_cast<std::uint32_t>(width));
    putBigEndian(header, static_cast<std::uint32_t>(height));
    header.insert(header.end(), {8, 2, 0, 0, 0}); // 8-bit RGB

    Bytes png = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    putChunk(png, "IHDR", header);
    putChunk(png, "IDAT", zlibStore(raw));
    putChunk(png, "IEND", {});
    return png;
}

void writePng(const std::string& path, int width, int height,
              const std::vector<std::uint8_t>& rgb) {
    const Bytes png = encodePng(width, height, rgb);
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(png.data()),
               static_cast<std::streamsize>(png.size()));
    if (!file) {
        throw std::runtime_error("Could not write " + path);
    }
}
