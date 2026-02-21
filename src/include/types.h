#ifndef TYPES_H
#define TYPES_H

#include <stddef.h>

typedef enum {
    PARSE_OK = 0,
    PARSE_IO,
    PARSE_BAD_SIG,
    PARSE_BAD_FORMAT,
    PARSE_UNSUPPORTED
} ParseError;

typedef struct {
    int width;
    int height;
    int components;
    int precision;
    int sof_marker;
} JpegInfo;

typedef struct {
    int error_code;
    const char *file_path;
    const char *file_type;
    size_t file_size;
    unsigned char signature[2];
} file_info;

typedef struct {
    int verbose;
    int info_mode;
    const char *path;
} AppOptions;

#endif
