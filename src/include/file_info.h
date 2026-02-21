#ifndef FILE_INFO_H
#define FILE_INFO_H

#include "types.h"

file_info check_file_type(const char *path);
int print_file_info(const file_info *fi);
int print_file_metadata(const char *path);

#endif
