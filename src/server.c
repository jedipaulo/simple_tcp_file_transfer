#include <stdio.h>
#include <string.h>
#include <stdlib.h> //atoi
#include <openssl/sha.h>

#include "sal.h"
#include "tlv.h"
#include "common.h"

/* ========================================================================== *
 * Data definitions                                                           *
 * ========================================================================== */
#define CONNECTION_QUEUE_SIZE 1

typedef struct {
    struct sockaddr_in addr;
    sal_socket_t listen_sock;
    char* storage_dir;
} server_data;

typedef struct {
    char file_path[MAX_PATH_LEN + 1];
    long file_size;
    sal_socket_t socket;
} connection_data;

/* ========================================================================== *
 * Forward declarations to avoid concerning about function definition order   *
 * ========================================================================== */
void print_usage(const char* app_name);
bool receive_header(const server_data* server_data, connection_data* connection_data);
bool receive_file_content(connection_data* connection_data);
bool receive_file(const server_data* server_data);
void send_ack(sal_socket_t socket);
void send_nack(sal_socket_t socket);
bool parse_input(const int argc, const char** argv, server_data* data);
void release_server_data(server_data* data);
bool start_listening(server_data* data);
void stop_listening(server_data* data);

/* ========================================================================== *
 * Main API                                                                   *
 * ========================================================================== */
int main(const int argc, const char** argv) {
    server_data data;
    bzero(&data, sizeof(data));

    if (!parse_input(argc, argv, &data)) {
        print_usage(argv[0]);
        return EXIT_CODE_ON_ERROR;
    }

    if (!start_listening(&data)) {
        release_server_data(&data);
        return EXIT_CODE_ON_ERROR;
    }

    /* Keep receiving requests */
    bool keep_running = true;
    while (keep_running) {
        receive_file(&data);
    }
    release_server_data(&data);

    return EXIT_CODE_ON_SUCCESS;
}

/* ========================================================================== *
 * Helper functions                                                           *
 * ========================================================================== */
/**
 * @brief Prints usage.
 *
 * @param app_name The name of running app
 *
 * @return No return
 **/
void print_usage(const char* app_name) {
    fprintf(
        stderr,
        "Usage: %s <storage directory> <listening IP address> <listening port>\n",
        app_name
    );
}

/**
 * @brief Receives TLV with header information.
 *
 * @param server_data The server internal data
 * @param connection_data The connection-specific internal data
 *
 * @return true if header information was received successfully
 * @return false otherwise
 **/
bool receive_header(const server_data* server_data, connection_data* connection_data) {
    tlv_t tlv_header = {0};
    if (!receive_tlv_data(connection_data->socket, &tlv_header)) {
        return false;
    }
    if (get_tlv_type(&tlv_header) != TLV_TYPE_HEADER) {
        set_error_description("No header received");
        print_error("Protocol error");
        goto RELEASE_TLVS;
    }
    uint16_t offset = 0;
    tlv_t sub_tlv_file_name = {0};
    if (!parse_tlv(&tlv_header.buffer[offset], &sub_tlv_file_name)) {
        goto RELEASE_TLVS;
    }
    offset += get_tlv_length(&sub_tlv_file_name) + TLV_HEADER_LENGTH;
    if (get_tlv_type(&sub_tlv_file_name) != TLV_TYPE_FILE_NAME) {
        set_error_description("No file name received");
        print_error("Protocol error");
        goto RELEASE_TLVS;
    }
    if (get_tlv_length(&sub_tlv_file_name) == 0 ||
        strlen(server_data->storage_dir) + 1 + get_tlv_length(&sub_tlv_file_name) > MAX_PATH_LEN) {
        reset_error_description();
        print_error("Invalid filename");
        goto RELEASE_TLVS;
    }

    strcpy(connection_data->file_path, server_data->storage_dir);
    strcat(connection_data->file_path, "/"),
    strncat(connection_data->file_path, get_tlv_value_raw(&sub_tlv_file_name), MAX_PATH_LEN);

    tlv_t sub_tlv_file_size = {0};
    parse_tlv(&tlv_header.buffer[offset], &sub_tlv_file_size);
    offset += get_tlv_length(&sub_tlv_file_size) + TLV_HEADER_LENGTH;
    if (get_tlv_type(&sub_tlv_file_size) != TLV_TYPE_FILE_SIZE) {
        set_error_description("No file size received");
        print_error("Protocol error");
        goto RELEASE_TLVS;
    }
    connection_data->file_size = get_tlv_value_long(&sub_tlv_file_size);
    tlv_release_tlvs();
    return true;

RELEASE_TLVS:
    tlv_release_tlvs();
    return false;
}

/**
 * @brief Receives file content and digest.
 * The received data is written to file and validated against a provided
 * checksum to ensure there was no transmission error.
 *
 * @param connection_data The connection-specific internal data
 *
 * @return true if file content was received, written and validated successfully
 * @return false otherwise
 **/
bool receive_file_content(connection_data* connection_data) {
    FILE* fp = fopen(connection_data->file_path, "w+b");
    if (fp == NULL) {
        print_error("Opening file failed");
        return false;
    }

    SHA512_CTX sha512_ctx;
    SHA512_Init(&sha512_ctx);
    static uint8_t sha512_buffer[SHA512_DIGEST_LENGTH] = {0};
    long received_bytes = 0;
    bool eof = false;
    while (!eof) {
        tlv_t tlv = {0};
        if (!receive_tlv_data(connection_data->socket, &tlv)) {
            goto CLOSE_FILE;
        }
        uint16_t length = get_tlv_length(&tlv);
        switch (get_tlv_type(&tlv)) {
        case TLV_TYPE_FILE_CONTENT:
            if (fwrite(get_tlv_value_raw(&tlv), 1, length, fp) != length) {
                goto RELEASE_TLVS;
            }
            SHA512_Update(&sha512_ctx, get_tlv_value_raw(&tlv), length);
            received_bytes += length;
            break;
        case TLV_TYPE_CHECKSUM_SHA512:
            eof = true;
            fclose(fp);
            fp = NULL;
            SHA512_Final(sha512_buffer, &sha512_ctx);
            if (memcmp(sha512_buffer, get_tlv_value_raw(&tlv), SHA512_DIGEST_LENGTH) != 0) {
                reset_error_description();
                print_error("File validation failed");
                goto CHECKSUM_ERROR;
            }
            break;
        default:
            set_error_description("Unknown TLV %d", get_tlv_type(&tlv));
            print_error("Protocol error");
            goto RELEASE_TLVS;
        }
        tlv_release_tlvs();
    }
    send_ack(connection_data->socket);
    return true;

RELEASE_TLVS:
    tlv_release_tlvs();
CLOSE_FILE:
    fclose(fp);
    fp = NULL;
CHECKSUM_ERROR:
    tlv_release_tlvs();
    send_nack(connection_data->socket);
    return false;
}

/**
 * @brief Accepts an incoming connection and receives a file through it.
 *
 * @param server_data The server internal data
 *
 * @return true if file content was received, written and validated successfully
 * @return false otherwise
 **/
bool receive_file(const server_data* server_data) {
    connection_data connection_data;
    bzero(&connection_data, sizeof(connection_data));
    if ((connection_data.socket = sal_accept(server_data->listen_sock)) == NULL) {
        return false;
    }

    if (!receive_header(server_data, &connection_data)) {
        goto RELEASE_CONNECTION;
    }
    print_msg(
        "Receiving file \"%s\" containing %ld bytes...",
        connection_data.file_path,
        connection_data.file_size
    );
    fflush(stdout);
    if (!receive_file_content(&connection_data)) {
        goto PRINT_ERROR;
    }

    sal_close(connection_data.socket);
    sal_destroy_socket(connection_data.socket);
    connection_data.socket = NULL;
    print_msg(" done\n");
    return true;

PRINT_ERROR:
    print_msg(" error\n");
RELEASE_CONNECTION:
    sal_close(connection_data.socket);
    sal_destroy_socket(connection_data.socket);
    connection_data.socket = NULL;

    return false;
}

/**
 * @brief Replies that file was received successfully.
 *
 * @param socket The used socket
 *
 * @return No return
 **/
void send_ack(sal_socket_t socket) {
    tlv_t tlv_content = new_tlv(TLV_TYPE_ACK, 0);
    if (!send_tlv_data(socket, &tlv_content)) {
        print_warning("Ack reply failed");
    }
    tlv_release_tlvs();
}

/**
 * @brief Replies that file was not received successfully.
 *
 * @param socket The used socket
 *
 * @return No return
 **/
void send_nack(sal_socket_t socket) {
    tlv_t tlv_content = new_tlv(TLV_TYPE_NACK, 0);
    if (!send_tlv_data(socket, &tlv_content)) {
        print_warning("Nack reply failed");
    }
    tlv_release_tlvs();
}

/**
 * @brief Parses input arguments and validate them.
 *
 * @param argc The number of arguments
 * @param argv The arguments values
 * @param[out] data The server internal data
 *
 * @return true if given arguments are valid
 * @return false otherwise
 **/
bool parse_input(const int argc, const char** argv, server_data* data) {
    if (argc != 4) {
        return false;
    }

    const char* storage_dir = argv[1];
    switch (sal_is_dir_writable(storage_dir)) {
    case SAL_DIR_NOT_FOUND:
        set_error_description("%s", storage_dir);
        print_error("Storage directory not found");
        return false;
        break;
    case SAL_DIR_NOT_WRITABLE:
        set_error_description("%s", storage_dir);
        print_error("Storage directory is not writable");
        return false;
        break;
    default:
        break;
    }

    struct in_addr server_ip_addr = {0};
    if (inet_aton(argv[2], &server_ip_addr) == 0) {
        set_error_description("%s", argv[2]);
        print_error("Invalid listening IP");
        return false;
    }

    const int server_port = atoi(argv[3]);
    if ((server_port <= 0) || (server_port > 65535)) {
        set_error_description("%d", server_port);
        print_error("Invalid listening port");
        return false;
    }

    data->storage_dir = strdup(storage_dir);
    data->addr.sin_addr = server_ip_addr;
    data->addr.sin_port = htons(server_port);
    data->addr.sin_family = AF_INET;

    return true;
}

/**
 * @brief Releases server internal data.
 *
 * @param data The server internal data
 *
 * @return true if given arguments are valid
 * @return false otherwise
 **/
void release_server_data(server_data* data) {
    if (data->listen_sock) {
        stop_listening(data);
        sal_destroy_socket(data->listen_sock);
        data->listen_sock = NULL;
    }
    if (data->storage_dir) {
        free(data->storage_dir);
        data->storage_dir = NULL;
    }
}

/**
 * @brief Creates a listening socket for receiving connection requests.
 *
 * @param server_data The server internal data
 *
 * @return true if given arguments are valid
 * @return false otherwise
 **/
bool start_listening(server_data* data) {
    if ((data->listen_sock = sal_create_socket()) == NULL) {
        return false;
    }

    if ((sal_bind(data->listen_sock, &data->addr)) != SAL_OK) {
        goto RELEASE_SOCKET;
    }

    if (sal_listen(data->listen_sock, CONNECTION_QUEUE_SIZE) != 0) {
        goto RELEASE_SOCKET;
    }
    return true;

RELEASE_SOCKET:
    sal_close(data->listen_sock);
    sal_destroy_socket(data->listen_sock);
    data->listen_sock = NULL;

    return false;
}

/**
 * @brief Stops the listening socket.
 *
 * @param server_data The server internal data
 *
 * @return true if given arguments are valid
 * @return false otherwise
 **/
void stop_listening(server_data* data) {
    if (data->listen_sock == NULL) {
        return;
    }
    sal_close(data->listen_sock);
}
