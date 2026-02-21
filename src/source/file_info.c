#include <stdio.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#include "file_info.h"

static void mode_to_string(mode_t mode, char out[11]) {
    out[0] = S_ISDIR(mode) ? 'd' : '-';
    out[1] = (mode & S_IRUSR) ? 'r' : '-';
    out[2] = (mode & S_IWUSR) ? 'w' : '-';
    out[3] = (mode & S_IXUSR) ? 'x' : '-';
    out[4] = (mode & S_IRGRP) ? 'r' : '-';
    out[5] = (mode & S_IWGRP) ? 'w' : '-';
    out[6] = (mode & S_IXGRP) ? 'x' : '-';
    out[7] = (mode & S_IROTH) ? 'r' : '-';
    out[8] = (mode & S_IWOTH) ? 'w' : '-';
    out[9] = (mode & S_IXOTH) ? 'x' : '-';
    out[10] = '\0';
}

static void format_time(time_t ts, char *out, size_t out_size) {
    struct tm *tm_ptr = localtime(&ts);
    if (tm_ptr == NULL) {
        snprintf(out, out_size, "unknown");
        return;
    }
    strftime(out, out_size, "%Y-%m-%d %H:%M:%S %z", tm_ptr);
}

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

int print_file_metadata(const char *path) {
    if (path == NULL) {
        fprintf(stderr, "print_file_metadata: path == NULL\n");
        return 1;
    }

    struct stat st;
    if (stat(path, &st) != 0) {
        fprintf(stderr, "stat(%s) failed: %s\n", path, strerror(errno));
        return 1;
    }

    char perms[11];
    char atime_buf[32];
    char mtime_buf[32];
    char ctime_buf[32];
    mode_to_string(st.st_mode, perms);
    format_time(st.st_atime, atime_buf, sizeof(atime_buf));
    format_time(st.st_mtime, mtime_buf, sizeof(mtime_buf));
    format_time(st.st_ctime, ctime_buf, sizeof(ctime_buf));

    struct passwd *pw = getpwuid(st.st_uid);
    struct group *gr = getgrgid(st.st_gid);
    const char *user = (pw && pw->pw_name) ? pw->pw_name : "unknown";
    const char *group = (gr && gr->gr_name) ? gr->gr_name : "unknown";

    printf("File metadata:\n");
    printf("- Permissions: %s (%04o)\n", perms, (unsigned)(st.st_mode & 07777));
    printf("- Owner: %s (uid=%u)\n", user, (unsigned)st.st_uid);
    printf("- Group: %s (gid=%u)\n", group, (unsigned)st.st_gid);
    printf("- Last access: %s\n", atime_buf);
    printf("- Last modification: %s\n", mtime_buf);
    printf("- Status change (ctime): %s\n", ctime_buf);

    return 0;
}
