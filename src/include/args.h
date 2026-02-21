#ifndef ARGS_H
#define ARGS_H

#include "types.h"

int parse_args(int argc, char *argv[], AppOptions *opts);
void print_usage(const char *prog);

#endif
