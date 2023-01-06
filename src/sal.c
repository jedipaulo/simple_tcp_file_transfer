#include <unistd.h> //access
#include <sys/stat.h> //stat
#include <sys/socket.h>
#include <libgen.h> //basename
#include <string.h> //strdup
#include <string.h> //strdup
#include <errno.h>

#include "sal.h"
#include "sal_imp.h"
#include "common.h"

sal_ret sal_is_dir_writable(const char* dir) {
    return sal_imp_is_dir_writable(dir);
}

sal_ret sal_is_file_readable(const char* path) {
    return sal_imp_is_file_readable(path);
}

char* sal_get_filename(const char* path) {
    char* ret = NULL;
    if ((ret = sal_imp_get_filename(path)) == NULL) {
        print_error("Get file name failed");
    }
    return ret;
}

sal_socket_t sal_create_socket() {
    sal_socket_t ret = SAL_OK;
    if ((ret = sal_imp_create_socket()) == NULL) {
        print_error("Socket creation failed");
    }
    return ret;
}

void sal_destroy_socket(sal_socket_t socket) {
    sal_imp_destroy_socket(socket);
}

sal_ret sal_connect(sal_socket_t socket, struct sockaddr_in* target_addr) {
    sal_ret ret = SAL_OK;
    if ((ret = sal_imp_connect(socket, target_addr)) != SAL_OK) {
        print_error("Connect failed");
    }
    return ret;
}

sal_socket_t sal_accept(sal_socket_t listening_socket) {
    sal_socket_t ret = NULL;
    if ((ret = sal_imp_accept(listening_socket)) == NULL) {
        print_error("Accept failed");
    }
    return ret;
}

sal_ret sal_bind(sal_socket_t socket, struct sockaddr_in* addr) {
    sal_ret ret = SAL_OK;
    if ((ret = sal_imp_bind(socket, addr)) != SAL_OK) {
        print_error("Bind failed");
    }
    return ret;
}

sal_ret sal_listen(sal_socket_t socket, int connection_queue_size) {
    sal_ret ret = SAL_OK;
    if ((ret = sal_imp_listen(socket, connection_queue_size)) != SAL_OK) {
        print_error("Listen failed");
    }
    return ret;
}

sal_ret sal_close(sal_socket_t socket) {
    sal_ret ret = SAL_OK;
    if ((ret = sal_imp_close(socket)) != SAL_OK) {
        print_error("Close failed");
    }
    return ret;
}

sal_ret sal_send_msg(sal_socket_t socket, const uint8_t* buffer, const uint16_t length) {
    sal_ret ret = SAL_OK;
    if ((ret = sal_imp_send_msg(socket, buffer, length)) != SAL_OK) {
        print_error("Send failed");
    }
    return ret;
}

sal_ret sal_receive_msg(sal_socket_t socket, uint8_t* buffer, const uint16_t length) {
    sal_ret ret = SAL_OK;
    if ((ret = sal_imp_receive_msg(socket, buffer, length)) != SAL_OK) {
        print_error("Received failed");
    }
    return ret;
}
