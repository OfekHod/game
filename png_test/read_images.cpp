/*
See:
http://www.libpng.org/pub/png/libpng-1.2.5-manual.html
*/
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

void
read_png_file_(PngImage *img, const char *file_name, FILE *fp) {

  // Setup
  img->read_error = false;
  unsigned char header[8];
  if (fread(header, 1, 8, fp) != 8) {
    fprintf(stderr, "Bad png format %s\n", file_name);
    img->read_error = true;
    return;
  }

  if (png_sig_cmp(header, 0, 8)) {
    fprintf(stderr, "Bad png format %s\n", file_name);
    img->read_error = true;
    return;
  }

  png_structp png_ptr =
      png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);

  if (!png_ptr) {
    img->read_error = true;
    return;
  }

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_read_struct(&png_ptr, nullptr, nullptr);
    img->read_error = true;
    return;
  }

  if (setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
    img->read_error = true;
    return;
  }

  png_init_io(png_ptr, fp);
  png_set_sig_bytes(png_ptr, 8);
  png_read_info(png_ptr, info_ptr);

  // Read fields

  img->width = png_get_image_width(png_ptr, info_ptr);
  img->height = png_get_image_height(png_ptr, info_ptr);
  img->color_type = png_get_color_type(png_ptr, info_ptr);
  img->bit_depth = png_get_bit_depth(png_ptr, info_ptr);
  img->number_of_passes = png_set_interlace_handling(png_ptr);

  // Read pixels
  const int row_size = png_get_rowbytes(png_ptr, info_ptr);
  img->pixels = (png_byte *)malloc(row_size * img->height);
  png_byte **row_pointers =
      (png_byte **)malloc(sizeof(png_byte *) * img->height);
  for (int row = 0; row < img->height; row++) {
    row_pointers[row] = img->pixels + row * row_size;
  }
  png_read_image(png_ptr, row_pointers);

  // Free resources
  free(row_pointers);
  png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
}

PngImage
read_png_file(const char *file_name) {
  PngImage img;

  FILE *fp = fopen(file_name, "rb");
  if (fp == nullptr) {
    fprintf(stderr, "File %s read problem : %s\n", file_name, strerror(errno));

    img.read_error = true;
    return img;
  }

  read_png_file_(&img, file_name, fp);
  fclose(fp);
  return img;
}

int main(int argc, char* argv[]) {
    PngImage img = read_png_file("../texture1.png");
    printf("Read image w: %d h: %d\n", img.width, img.height);
    return 0;
}
