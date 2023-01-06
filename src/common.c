#include <stdio.h> //printf
#include <stdlib.h> //exit
#include <errno.h> //errno
#include <string.h>
#include <stdarg.h> //vargs

#include "common.h"

static char root_error_log_buffer[LOG_MSG_MAX_LENGTH + 1];

void set_error_description(const char * format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(root_error_log_buffer, LOG_MSG_MAX_LENGTH, format, args);
    va_end(args);
}

void reset_error_description() {
    root_error_log_buffer[0] = '\0';
}

void print_error(const char *msg) {
    if (strlen(root_error_log_buffer) == 0) {
        fprintf(stderr, "[ERROR] %s\n", msg);
    } else {
        fprintf(stderr, "[ERROR] %s: %s\n", msg, root_error_log_buffer);
        reset_error_description();
    }
}

void print_warning(const char *msg) {
    if (strlen(root_error_log_buffer) == 0) {
        fprintf(stderr, "[WARNING] %s\n", msg);
    } else {
        fprintf(stderr, "[WARNING] %s: %s\n", msg, root_error_log_buffer);
        reset_error_description();
    }
}

void print_msg(const char * format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}
