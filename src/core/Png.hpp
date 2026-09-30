#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Minimal PNG encoder: 8-bit RGB, uncompressed deflate blocks
std::vector<std::uint8_t> encodePng(int width, int height,
                                    const std::vector<std::uint8_t>& rgb);

// Rows top to bottom; throws on I/O failure
void writePng(const std::string& path, int width, int height,
              const std::vector<std::uint8_t>& rgb);
