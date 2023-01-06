#ifndef _SAL_H_
#define _SAL_H_

#include <stdbool.h>
#include <stdint.h>
#include <arpa/inet.h>

#define MSG_BUFFER_LEN 1024
#define MAX_PATH_LEN 1024
#if MSG_BUFFER_LEN < 5
#error Buffer is too small
#endif

typedef enum {
    SAL_OK,
    SAL_ERROR,
    SAL_DIR_NOT_FOUND,
    SAL_DIR_NOT_WRITABLE,
    SAL_FILE_NOT_FOUND,
    SAL_FILE_NOT_READABLE
} sal_ret;

typedef void* sal_socket_t;

/**
 * @brief Checks if a given directory exists and is writable.
 *
 * @param dir The directory to be verified
 *
 * @return SAL_OK if the given directory exists and is writable
 * @return SAL_DIR_NOT_FOUND, if the given directory doesn't exist
 * @return SAL_DIR_NOT_WRITABLE, if the given directory exists but is not writable
 **/
sal_ret sal_is_dir_writable(const char* dir);

/**
 * @brief Checks if a given file exists and is readable.
 *
 * @param path The file path
 *
 * @return SAL_OK if the given file exists and is readable
 * @return SAL_FILE_NOT_FOUND, if the given file doesn't exist
 * @return SAL_FILE_NOT_READABLE, if the given file exists but is not readable
 **/
sal_ret sal_is_file_readable(const char* path);

/**
 * @brief Gets the file name from the file path.
 * @note It returns a malloc'd value that must be freed by user.
 *
 * @param path The file path
 *
 * @return the file name
 **/
char* sal_get_filename(const char* path);

/**
 * @brief Creates a socket.
 * @note The created socket shall be released by sal_destroy_socket().
 *
 * @return the created socket
 **/
sal_socket_t sal_create_socket();

/**
 * @brief Releases a socket.
 *
 * @param socket The created socket.
 *
 * @return No return
 **/
void sal_destroy_socket(sal_socket_t socket);

/**
 * @brief Connects a socket.
 *
 * @param socket The used socket
 * @param target_addr The remote address
 *
 * @return SAL_OK if socket is connected successfully
 * @return SAL_ERROR otherwise
 **/
sal_ret sal_connect(sal_socket_t socket, struct sockaddr_in* target_addr);

/**
 * @brief Accepts an incoming connection on a socket.
 *
 * @param listening_socket The listening socket
 *
 * @return the created socket to handle the accepted connection
 * @return NULL otherwise
 **/
sal_socket_t sal_accept(sal_socket_t listening_socket);

/**
 * @brief Binds a socket to an address.
 *
 * @param socket The given socket
 * @param addr The address to be bound to
 *
 * @return SAL_OK if socket is bound successfully
 * @return SAL_ERROR otherwise
 **/
sal_ret sal_bind(sal_socket_t socket, struct sockaddr_in* addr);

/**
 * @brief Marks a socket that will be used for accepting incoming connections.
 *
 * @param socket The given socket
 * @param connection_queue_size The queue size for waiting incoming connections
 *
 * @return SAL_OK if socket is marked successfully
 * @return SAL_ERROR otherwise
 **/
sal_ret sal_listen(sal_socket_t socket, int connection_queue_size);

/**
 * @brief Closes a socket.
 *
 * @param socket The given socket
 *
 * @return SAL_OK if socket is closed successfully
 * @return SAL_ERROR otherwise
 **/
sal_ret sal_close(sal_socket_t socket);

/**
 * @brief Sends a message through the given socket.
 *
 * @param socket The used socket
 * @param buffer The data buffer
 * @param length The data buffer length
 *
 * @return SAL_OK if data was sent successfully
 * @return SAL_ERROR otherwise
 **/
sal_ret sal_send_msg(sal_socket_t socket, const uint8_t* buffer, const uint16_t length);

/**
 * @brief Receives a message from the given socket.
 *
 * @param socket The used socket
 * @param[out] buffer The data buffer
 * @param length The data buffer length
 *
 * @return SAL_OK if data was sent successfully
 * @return SAL_ERROR otherwise
 **/
sal_ret sal_receive_msg(sal_socket_t socket, uint8_t* buffer, const uint16_t length);

#endif /* _SAL_H_ */
