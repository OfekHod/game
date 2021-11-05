#pragma once

#include <png.h>

struct PngImage {
  int width;
  int height;
  png_byte color_type;
  png_byte bit_depth;
  int number_of_passes;
  png_byte *pixels;
  bool read_error = false;
};

PngImage read_png_file(char *file_name);
