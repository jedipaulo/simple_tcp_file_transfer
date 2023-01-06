#ifndef __COMMON_H__
#define __COMMON_H__

/* ========================================================================== *
 * Data definitions                                                           *
 * ========================================================================== */
#define LOG_MSG_MAX_LENGTH 1024

#define EXIT_CODE_ON_SUCCESS 0
#define EXIT_CODE_ON_ERROR 1

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

/**
 * @brief Clears previously set error description.
 *
 * @return No return
 */
void reset_error_description();

/**
 * @brief Set a error description that will be used on subsequent call to print_error().
 *
 * @param format The message format
 * @param ... The message arguments
 *
 * @return No return
 */
void set_error_description(const char * format, ...);

/**
 * @brief Prints an error message to the standard error. If there is an unused error description
 * previously set by set_error_description(), it will be appended to the given msg on print.
 *
 * @param msg The message to be printed
 *
 * @return No return
 */
void print_error(const char *msg);

/**
 * @brief Prints a warning message to the standard error. If there is an unused error description
 * previously set by set_error_description(), it will be appended to the given msg on print.
 *
 * @param msg The message to be printed
 *
 * @return No return
 */
void print_warning(const char *msg);

/**
 * @brief Prints a message to the standard output.
 *
 * @param format The message format
 * @param ... The message arguments
 *
 * @return No return
 */
void print_msg(const char * format, ...);

#endif /* __COMMON_H__ */
