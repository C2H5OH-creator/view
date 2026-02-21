#include <getopt.h>
#include <stdio.h>

#include "args.h"

static struct option long_opts[] = {
    {"help", no_argument, 0, 'h'},
    {"verbose", no_argument, 0, 'v'},
    {"info", no_argument, 0, 'i'},
    {"force-jpeg", no_argument, 0, 'f'},
    {0, 0, 0, 0}
};

void print_usage(const char *prog) {
    printf("Usage: %s [--verbose] [--info] [--force-jpeg] <image_file>\n", prog);
}

int parse_args(int argc, char *argv[], AppOptions *opts) {
    int opt;

    opts->verbose = 0;
    opts->info_mode = 0;
    opts->force_jpeg = 0;
    opts->path = NULL;

    while ((opt = getopt_long(argc, argv, "hvif", long_opts, NULL)) != -1) {
        switch (opt) {
            case 'h':
                print_usage(argv[0]);
                return 1;
            case 'v':
                opts->verbose = 1;
                break;
            case 'i':
                opts->info_mode = 1;
                break;
            case 'f':
                opts->force_jpeg = 1;
                break;
            default:
                return -1;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "Ошибка: укажи путь к файлу\n");
        return -1;
    }

    opts->path = argv[optind];
    return 0;
}
