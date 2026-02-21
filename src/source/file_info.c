#include <stdio.h>

#include "file_info.h"

file_info check_file_type(const char *path) {
    file_info info = {0};
    info.file_path = path;
    info.file_type = "unknown";

    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        info.error_code = 1;
        return info;
    }

    size_t bytes_read = fread(info.signature, 1, 2, file);
    if (bytes_read != 2) {
        info.error_code = 2;
        fclose(file);
        return info;
    }

    if (fseek(file, 0, SEEK_END) == 0) {
        long sz = ftell(file);
        if (sz >= 0) {
            info.file_size = (size_t)sz;
        }
    }

    if (info.signature[0] == 0xFF && info.signature[1] == 0xD8) {
        info.file_type = "jpeg";
        info.error_code = 0;
    } else {
        info.file_type = "unknown";
        info.error_code = 3;
    }

    fclose(file);
    return info;
}

int print_file_info(const file_info *fi) {
    if (fi == NULL) {
        fprintf(stderr, "print_file_info: fi == NULL\n");
        return 1;
    }

    printf("1. Path: %s\n", fi->file_path ? fi->file_path : "(null)");
    printf("2. File size: %zu bytes\n", fi->file_size);
    printf("3. File type: %s\n", fi->file_type ? fi->file_type : "unknown");
    printf("4. File signature: %02X %02X\n", fi->signature[0], fi->signature[1]);

    return 0;
}
