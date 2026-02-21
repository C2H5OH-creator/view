#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "args.h"
#include "file_info.h"
#include "jpeg.h"
#include "window.h"

static const char *file_basename(const char *path) {
    const char *slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
}

int main(int argc, char *argv[]) {
    AppOptions opts;
    int rc = parse_args(argc, argv, &opts);

    if (rc == 1) return 0;
    if (rc != 0) return 1;

    file_info info = check_file_type(opts.path);
    if (opts.verbose) {
        print_file_info(&info);
    }

    if (info.error_code != 0) {
        fprintf(stderr, "Ошибка обработки файла (code=%d)\n", info.error_code);
        return 1;
    }

    if (!(info.file_type != NULL && strcmp(info.file_type, "jpeg") == 0)) {
        printf("Файл — не JPEG. Его поддержки пока нет\n");
        return 1;
    }

    FILE *f = fopen(opts.path, "rb");
    if (!f) {
        perror("fopen");
        return 1;
    }

    JpegInfo ji;
    ParseError err = parse_jpeg(f, &ji, opts.verbose);
    fclose(f);

    if (err != PARSE_OK) {
        fprintf(stderr, "JPEG parse error: %d\n", err);
        return 1;
    }

    if (opts.info_mode) {
        printf("JPEG: %dx%d, components=%d, precision=%d, SOF=0x%X\n",
               ji.width, ji.height, ji.components, ji.precision, ji.sof_marker);
    }

    unsigned char *rgb = NULL;
    int image_w = 0;
    int image_h = 0;
    if (decode_jpeg_rgb(opts.path, &rgb, &image_w, &image_h) != 0) {
        fprintf(stderr, "Ошибка декодирования JPEG в RGB\n");
        return 1;
    }

    rc = show_window_with_image(file_basename(opts.path), rgb, image_w, image_h);
    free(rgb);
    return rc;
}
