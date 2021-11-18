#pragma once

#include <png.h>
#include <cstdint>

struct Image {
  uint32_t width;
  uint32_t height;
  uint8_t *pixels;
  bool is_default;
};

Image
read_png_file(const char *file_name);
