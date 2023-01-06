#ifndef _SAL_HPP_
#define _SAL_HPP_

#include <stdbool.h>

typedef enum {
    SAL_OK,
    SAL_DIR_NOT_FOUND,
    SAL_DIR_NOT_WRITABLE,
    SAL_FILE_NOT_FOUND,
    SAL_FILE_NOT_READABLE
} sal_dir_status;

/* @brief Checks if a given directory exists.
   @param dir The directory to be verified.
   @return
*/
sal_dir_status sal_dir_exists(const char* dir);

/* @brief Checks if a given file exists.
   @param path The file path to be verified.
   @return
*/
sal_dir_status sal_file_exists(const char* path);

#endif /* _SAL_HPP_ */
