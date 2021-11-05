/*
See:
http://www.libpng.org/pub/png/libpng-1.2.5-manual.html
*/
#include "read_images.hpp"
#include <stdio.h>
#include <stdlib.h>

PngImage read_png_file(char *file_name) {
  FILE *fp = fopen(file_name, "rb");

  unsigned char header[8];
  fread(header, 1, 8, fp);

  if (png_sig_cmp(header, 0, 8)) {
    fprintf(stderr, "Bad png format %s\n");
    PngImage res;
    res.read_error = true;
    return res;
  }

  png_structp png_ptr =
      png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);

  png_infop info_ptr = png_create_info_struct(png_ptr);

  PngImage img;

  [&png_ptr, &info_ptr, &img, &fp] {
    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, 8);
    png_read_info(png_ptr, info_ptr);

    img.width = png_get_image_width(png_ptr, info_ptr);
    img.height = png_get_image_height(png_ptr, info_ptr);
    img.color_type = png_get_color_type(png_ptr, info_ptr);
    img.bit_depth = png_get_bit_depth(png_ptr, info_ptr);
    img.number_of_passes = png_set_interlace_handling(png_ptr);

    const int row_size = png_get_rowbytes(png_ptr, info_ptr);
    img.pixels = (png_byte *)malloc(row_size * img.height);

    {
      png_byte **row_pointers =
          (png_byte **)malloc(sizeof(png_byte *) * img.height);
      for (int row = 0; row < img.height; row++) {
        row_pointers[row] = img.pixels + row * row_size;
      }
      png_read_image(png_ptr, row_pointers);
      free(row_pointers);
    }
  }();

  fclose(fp);

  return img;
}
