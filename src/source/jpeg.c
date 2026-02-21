#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <jpeglib.h>

#include "jpeg.h"

// Читает 8-битное целое число из файла
static int read_u8(FILE *f, uint8_t *out) {
    return fread(out, 1, 1, f) == 1 ? 0 : -1;
}

// Читает 16-битное целое число в большом порядке байтов из файла
static int read_u16_be(FILE *f, uint16_t *out) {
    uint8_t b[2];
    if (fread(b, 1, 2, f) != 2) return -1;
    *out = (uint16_t)((b[0] << 8) | b[1]);
    return 0;
}

// Пропускает n байт в файле
static int skip_n(FILE *f, uint16_t n) {
    return fseek(f, n, SEEK_CUR) == 0 ? 0 : -1;
}

// Проверяет, является ли маркер SOF
static int is_sof_marker(uint8_t m) {
    switch (m) {
        case 0xC0: case 0xC1: case 0xC2: case 0xC3:
        case 0xC5: case 0xC6: case 0xC7:
        case 0xC9: case 0xCA: case 0xCB:
        case 0xCD: case 0xCE: case 0xCF:
            return 1;
        default:
            return 0;
    }
}

static const char *marker_name(uint8_t marker) {
    switch (marker) {
        case 0xD8: return "SOI";
        case 0xD9: return "EOI";
        case 0xDA: return "SOS";
        case 0xC0: return "SOF0";
        case 0xC1: return "SOF1";
        case 0xC2: return "SOF2";
        case 0xC4: return "DHT";
        case 0xDB: return "DQT";
        case 0xDD: return "DRI";
        case 0xE0: return "APP0";
        case 0xE1: return "APP1";
        case 0xFE: return "COM";
        default: return "MARKER";
    }
}

static ParseError scan_entropy_until_marker(FILE *f, uint8_t *next_marker, int verbose) {
    uint8_t b;
    while (1) {
        if (read_u8(f, &b)) return PARSE_IO;
        if (b != 0xFF) continue;

        uint8_t marker;
        do {
            if (read_u8(f, &marker)) return PARSE_IO;
        } while (marker == 0xFF);

        if (marker == 0x00) {
            continue;
        }

        if (marker >= 0xD0 && marker <= 0xD7) {
            if (verbose) printf("[jpeg] marker 0xFF%02X (RST)\n", marker);
            continue;
        }

        if (verbose) {
            printf("[jpeg] marker 0xFF%02X (%s)\n", marker, marker_name(marker));
        }
        *next_marker = marker;
        return PARSE_OK;
    }
}

ParseError parse_jpeg(FILE *f, JpegInfo *out, int verbose) {
    uint8_t b0, b1;
    if (read_u8(f, &b0) || read_u8(f, &b1)) return PARSE_IO;
    if (!(b0 == 0xFF && b1 == 0xD8)) return PARSE_BAD_SIG;
    if (verbose) printf("[jpeg] marker 0xFFD8 (%s)\n", marker_name(0xD8));

    out->width = 0;
    out->height = 0;
    out->components = 0;
    out->precision = 0;
    out->sof_marker = -1;

    uint8_t marker = 0;
    int have_marker = 0;

    while (1) {
        if (!have_marker) {
            uint8_t ff;
            do {
                if (read_u8(f, &ff)) return PARSE_IO;
            } while (ff != 0xFF);

            do {
                if (read_u8(f, &marker)) return PARSE_IO;
            } while (marker == 0xFF);
        } else {
            have_marker = 0;
        }

        if (marker == 0xD9) {
            if (verbose) printf("[jpeg] marker 0xFF%02X (%s)\n", marker, marker_name(marker));
            return (out->width > 0 && out->height > 0) ? PARSE_OK : PARSE_BAD_FORMAT;
        }

        if (marker == 0x01 || (marker >= 0xD0 && marker <= 0xD7)) {
            continue;
        }

        uint16_t seg_len;
        if (read_u16_be(f, &seg_len)) return PARSE_IO;
        if (seg_len < 2) return PARSE_BAD_FORMAT;

        uint16_t payload = (uint16_t)(seg_len - 2);
        if (verbose) {
            printf("[jpeg] marker 0xFF%02X (%s), segment_len=%u, payload=%u\n",
                   marker, marker_name(marker), seg_len, payload);
        }

        if (marker == 0xDA) {
            if (payload > 0 && skip_n(f, payload)) return PARSE_IO;
            if (scan_entropy_until_marker(f, &marker, verbose) != PARSE_OK) return PARSE_IO;
            have_marker = 1;
            continue;
        }

        if (is_sof_marker(marker)) {
            uint8_t p, nf;
            uint16_t y, x;

            if (payload < 6) return PARSE_BAD_FORMAT;
            if (read_u8(f, &p) || read_u16_be(f, &y) || read_u16_be(f, &x) || read_u8(f, &nf)) {
                return PARSE_IO;
            }

            out->precision = p;
            out->height = y;
            out->width = x;
            out->components = nf;
            out->sof_marker = marker;

            if (verbose) {
                printf("[jpeg] SOF parsed: width=%d height=%d components=%d precision=%d\n",
                       out->width, out->height, out->components, out->precision);
            }

            if (payload > 6 && skip_n(f, (uint16_t)(payload - 6))) return PARSE_IO;
        } else {
            if (payload > 0 && skip_n(f, payload)) return PARSE_IO;
        }
    }
}

typedef struct {
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
} JpegError;

static void jpeg_error_exit(j_common_ptr cinfo) {
    JpegError *err = (JpegError *)cinfo->err;
    longjmp(err->setjmp_buffer, 1);
}

int decode_jpeg_rgb(const char *path, unsigned char **out_rgb, int *out_w, int *out_h) {
    if (!path || !out_rgb || !out_w || !out_h) return 1;

    *out_rgb = NULL;
    *out_w = 0;
    *out_h = 0;

    FILE *file = fopen(path, "rb");
    if (!file) {
        perror("fopen");
        return 1;
    }

    struct jpeg_decompress_struct cinfo;
    JpegError jerr;

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = jpeg_error_exit;

    if (setjmp(jerr.setjmp_buffer)) {
        jpeg_destroy_decompress(&cinfo);
        fclose(file);
        free(*out_rgb);
        *out_rgb = NULL;
        *out_w = 0;
        *out_h = 0;
        return 1;
    }

    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, file);

    jpeg_read_header(&cinfo, TRUE);
    cinfo.out_color_space = JCS_RGB;
    jpeg_start_decompress(&cinfo);

    int width = (int)cinfo.output_width;
    int height = (int)cinfo.output_height;
    int channels = (int)cinfo.output_components;
    if (channels != 3 || width <= 0 || height <= 0) {
        jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);
        fclose(file);
        return 1;
    }

    size_t row_stride = (size_t)width * (size_t)channels;
    size_t total_size = row_stride * (size_t)height;
    unsigned char *rgb = (unsigned char *)malloc(total_size);
    if (!rgb) {
        jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);
        fclose(file);
        return 1;
    }

    while (cinfo.output_scanline < cinfo.output_height) {
        unsigned char *row_ptr = rgb + ((size_t)cinfo.output_scanline * row_stride);
        JSAMPROW row[1] = { row_ptr };
        jpeg_read_scanlines(&cinfo, row, 1);
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(file);

    *out_rgb = rgb;
    *out_w = width;
    *out_h = height;
    return 0;
}
