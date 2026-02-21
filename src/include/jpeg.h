#ifndef JPEG_H
#define JPEG_H

#include <stdio.h>

#include "types.h"

ParseError parse_jpeg(FILE *f, JpegInfo *out, int verbose);
int decode_jpeg_rgb(const char *path, unsigned char **out_rgb, int *out_w, int *out_h);

#endif
