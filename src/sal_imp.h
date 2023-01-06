#ifndef __SAL_IMP_H__
#define __SAL_IMP_H__

#include "sal.h"

/**
 * @brief Implements sal_is_dir_writable()
 * @see sal_is_dir_writable()
 */
sal_ret sal_imp_is_dir_writable(const char* dir);

/**
 * @brief Implements sal_is_file_readable()
 * @see sal_is_file_readable()
 */
sal_ret sal_imp_is_file_readable(const char* path);

/**
 * @brief Implements sal_get_filename()
 * @see sal_get_filename()
 */
char* sal_imp_get_filename(const char* path);

/**
 * @brief Implements sal_create_socket()
 * @see sal_create_socket()
 */
sal_socket_t sal_imp_create_socket();

/**
 * @brief Implements sal_destroy_socket()
 * @see sal_destroy_socket()
 */
void sal_imp_destroy_socket(sal_socket_t socket);

/**
 * @brief Implements sal_connect()
 * @see sal_connect()
 */
sal_ret sal_imp_connect(sal_socket_t socket, struct sockaddr_in* target_addr);

/**
 * @brief Implements sal_accept()
 * @see sal_accept()
 */
sal_socket_t sal_imp_accept(sal_socket_t listening_socket);

/**
 * @brief Implements sal_bind()
 * @see sal_bind()
 */
sal_ret sal_imp_bind(sal_socket_t socket, struct sockaddr_in* addr);

/**
 * @brief Implements sal_listen()
 * @see sal_listen()
 */
sal_ret sal_imp_listen(sal_socket_t socket, int connection_queue_size);

/**
 * @brief Implements sal_close()
 * @see sal_close()
 */
sal_ret sal_imp_close(sal_socket_t socket);

/**
 * @brief Implements sal_send_msg()
 * @see sal_send_msg()
 */
sal_ret sal_imp_send_msg(sal_socket_t socket, const uint8_t* buffer, const uint16_t length);

/**
 * @brief Implements sal_receive_msg()
 * @see sal_receive_msg()
 */
sal_ret sal_imp_receive_msg(sal_socket_t socket, uint8_t* buffer, const uint16_t length);

#endif /* __SAL_IMP_H__ */
