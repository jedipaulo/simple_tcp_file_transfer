#include <unistd.h> //access
#include <sys/stat.h> //stat
#include <sys/socket.h>
#include <libgen.h> //basename
#include <string.h> //strdup
#include <string.h> //strdup
#include <errno.h>
#include <stdlib.h>

#include "sal_imp.h"
#include "common.h"

sal_ret sal_imp_is_dir_writable(const char* dir) {
    if (access(dir, W_OK) == 0) {
        return SAL_OK;
    } else if (access(dir, F_OK) == 0) {
        return SAL_DIR_NOT_WRITABLE;
    } else {
        return SAL_DIR_NOT_FOUND;
    }
}

sal_ret sal_imp_is_file_readable(const char* path) {
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

char* sal_imp_get_filename(const char* path) {
    /* basename() can change path, so it's better to give it a copy of original value... */
    char* buffer_path = strdup(path);
    char* file_name = strdup(basename(buffer_path));
    free(buffer_path);
    buffer_path = NULL;
    return file_name;
}

sal_socket_t sal_imp_create_socket() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        set_error_description("%s", strerror(errno));
        return NULL;
    }
    int* socket = malloc(sizeof(int));
    *socket = sockfd;
    return socket;
}

void sal_imp_destroy_socket(sal_socket_t socket) {
    free(socket);
}

sal_ret sal_imp_connect(sal_socket_t socket, struct sockaddr_in* target_addr) {
    if (connect(*(int*)socket, (struct sockaddr*)target_addr, sizeof(*target_addr)) != 0) {
        set_error_description("%s", strerror(errno));
        return SAL_ERROR;
    }
    return SAL_OK;
}

sal_socket_t sal_imp_accept(sal_socket_t listening_socket) {
    int listening_sockfd = *(int*)listening_socket;
    struct sockaddr_in remote_addr;
    int remote_addr_len = sizeof(remote_addr);
    int connection_fd = accept(listening_sockfd, (struct sockaddr *)&remote_addr, &remote_addr_len);
    if (connection_fd == -1 ) {
        set_error_description("%s", strerror(errno));
        return NULL;
    }

    int* socket = malloc(sizeof(int));
    *socket = connection_fd;
    return socket;
}

sal_ret sal_imp_bind(sal_socket_t socket, struct sockaddr_in* addr) {
    if (bind(*(int*)socket, (struct sockaddr*)addr, sizeof(*addr)) != 0) {
        set_error_description("%s", strerror(errno));
        return SAL_ERROR;
    }
    return SAL_OK;
}

sal_ret sal_imp_listen(sal_socket_t socket, int connection_queue_size) {
    if (listen(*(int*)socket, connection_queue_size) != 0) {
        set_error_description("%s", strerror(errno));
        return SAL_ERROR;
    }
    return SAL_OK;
}

sal_ret sal_imp_close(sal_socket_t socket) {
    if (close(*(int*)socket) != 0) {
        set_error_description("%s", strerror(errno));
        return SAL_ERROR;
    }
    return SAL_OK;
}

sal_ret sal_imp_send_msg(sal_socket_t socket, const uint8_t* buffer, const uint16_t length) {
    int sockfd = *((int*)socket);
    uint16_t offset = 0;
    while (offset < length) {
        uint16_t transmitting_bytes = MIN(length - offset, MSG_BUFFER_LEN);
        if (send(sockfd, &buffer[offset], transmitting_bytes, 0) < 0) {
            set_error_description("%s", strerror(errno));
            return SAL_ERROR;
        }
        offset += transmitting_bytes;
    }
    return SAL_OK;
}

sal_ret sal_imp_receive_msg(sal_socket_t socket, uint8_t* buffer, const uint16_t length) {
    int sockfd = *((int*)socket);
    uint16_t offset = 0;
    while (offset < length) {
        uint16_t requested_bytes = MIN(length - offset, MSG_BUFFER_LEN);
        ssize_t bytes_received = 0;
        if ((bytes_received = recv(sockfd, &buffer[offset], requested_bytes, 0)) <= 0) {
            if (bytes_received) {
                set_error_description("%s", strerror(errno));
            } else {
                set_error_description("No data");
            }
            return SAL_ERROR;
        }
        offset += bytes_received;
    }
    return SAL_OK;
}
