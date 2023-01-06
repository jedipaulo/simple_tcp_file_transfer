#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "sal.h"

sal_dir_status sal_dir_exists(const char* dir) {
    if (access(dir, W_OK) == 0) {
        return SAL_OK;
    } else if (access(dir, F_OK) == 0) {
        return SAL_DIR_NOT_WRITABLE;
    } else {
        return SAL_DIR_NOT_FOUND;
    }
}

sal_dir_status sal_file_exists(const char* path) {
    if (access(path, R_OK) == 0) {
            struct stat path_stat;
            stat(path, &path_stat);
            if  (S_ISREG(path_stat.st_mode)) {
                return SAL_OK;
            } else {
                return SAL_FILE_NOT_FOUND;
            }
        return SAL_OK;
    } else if (access(path, F_OK) == 0) {
        return SAL_FILE_NOT_READABLE;
    } else {
        return SAL_FILE_NOT_FOUND;
    }
}
