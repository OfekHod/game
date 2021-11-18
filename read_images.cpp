/*
See:
http://www.libpng.org/pub/png/libpng-1.2.5-manual.html
*/
#include "read_images.hpp"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct PngHandler {
  int width;
  int height;
  png_byte color_type;
  png_byte bit_depth;
  int number_of_passes;
  png_byte *pixels;
  bool read_error = false;
};
void
read_png_file_(PngHandler *handler, const char *file_name, FILE *fp) {

  // Setup
  handler->read_error = false;
  unsigned char header[8];
  if (fread(header, 1, 8, fp) != 8) {
    fprintf(stderr, "Bad png format %s\n", file_name);
    handler->read_error = true;
    return;
  }

  if (png_sig_cmp(header, 0, 8)) {
    fprintf(stderr, "Bad png format %s\n", file_name);
    handler->read_error = true;
    return;
  }

  png_structp png_ptr =
      png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);

  if (!png_ptr) {
    handler->read_error = true;
    return;
  }

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_read_struct(&png_ptr, nullptr, nullptr);
    handler->read_error = true;
    return;
  }

  if (setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
    handler->read_error = true;
    return;
  }

  png_init_io(png_ptr, fp);
  png_set_sig_bytes(png_ptr, 8);
  png_read_info(png_ptr, info_ptr);

  // Read fields

  handler->width = png_get_image_width(png_ptr, info_ptr);
  handler->height = png_get_image_height(png_ptr, info_ptr);
  handler->color_type = png_get_color_type(png_ptr, info_ptr);
  handler->bit_depth = png_get_bit_depth(png_ptr, info_ptr);
  handler->number_of_passes = png_set_interlace_handling(png_ptr);

  // Read pixels
  const int row_size = png_get_rowbytes(png_ptr, info_ptr);
  handler->pixels = (png_byte *)malloc(row_size * handler->height);
  png_byte **row_pointers =
      (png_byte **)malloc(sizeof(png_byte *) * handler->height);
  for (int row = 0; row < handler->height; row++) {
    row_pointers[row] = handler->pixels + row * row_size;
  }
  png_read_image(png_ptr, row_pointers);

  // Free resources
  free(row_pointers);
  png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
}

Image
read_png_file(const char *file_name) {
  PngHandler handler;

  FILE *fp = fopen(file_name, "rb");
  if (fp == nullptr) {
    fprintf(stderr, "File %s read problem : %s\n", file_name, strerror(errno));

    handler.read_error = true;
  } else {
    // TODO: Make sure we get rgb images
    read_png_file_(&handler, file_name, fp);
    fclose(fp);
  }

  Image res;
  if (handler.read_error) {
    res.width = 1;
    res.height = 1;
    res.pixels = (uint8_t *)malloc(sizeof(uint8_t));
    res.is_default = true;
  } else {
    res.width = handler.width;
    res.height = handler.height;
    res.pixels = handler.pixels;
    res.is_default = false;
  }

  return res;
}
