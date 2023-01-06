#include <stdio.h>
#include <stdlib.h> //atoi
#include <string.h> //str functions
#include <openssl/sha.h> //sha512

#include "sal.h"
#include "common.h"
#include "tlv.h"

/* ========================================================================== *
 * Data definitions                                                           *
 * ========================================================================== */
typedef struct {
    struct sockaddr_in server_addr; ///< the remote server address
    char* path; ///< the file path
    sal_socket_t transmission_socket; ///< the transmission socket
} client_data;

/* ========================================================================== *
 * Forward declarations to avoid concerning about function definition order   *
 * ========================================================================== */
void print_usage(const char* app_name);
long get_filesize(FILE* fp);
bool send_header(client_data* data, FILE* fp);
bool send_file_content(sal_socket_t socket, FILE* fp);
void send_file(client_data* data);
bool check_reply(sal_socket_t socket);
bool parse_input(const int argc, const char** argv, client_data* data);
void release_client_data(client_data* data);

/* ========================================================================== *
 * Main API                                                                   *
 * ========================================================================== */
int main(const int argc, const char** argv) {
    /* Gather data from input. */
    client_data data;
    bzero(&data, sizeof(data));
    if (!parse_input(argc, argv, &data)) {
        print_usage(argv[0]);
        return EXIT_CODE_ON_ERROR;
    }

    /* Send the specified file and exit */
    send_file(&data);
    release_client_data(&data);

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
    reset_error_description();
    fprintf(
        stderr,
        "Usage: %s <file path> <destination IP address> <destination port>\n",
        app_name
    );
}

/**
 * @brief Gets the size of an opened file.
 *
 * @param fp The pointer to the opened file
 *
 * @returns the file size
 **/
long get_filesize(FILE* fp) {
    fpos_t position;
    fgetpos(fp, &position);
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fsetpos (fp, &position);
    return file_size;
}

/**
 * @brief Sends TLV with header information.
 *
 * @param data The client internal data
 * @param fp The pointer to the opened file
 *
 * @return true if header information was sent successfully
 * @return false otherwise
 **/
bool send_header(client_data* data, FILE* fp) {
    const long file_size = get_filesize(fp);
    char* filename = sal_get_filename(data->path);
    const int filename_len = strlen(filename);

    tlv_t tlv_header = new_tlv(TLV_TYPE_HEADER, 0);
    tlv_t sub_tlv_file_name = new_tlv(TLV_TYPE_FILE_NAME, filename_len);
    set_tlv_value_raw(&sub_tlv_file_name, filename);
    tlv_t sub_tlv_file_size = new_tlv(TLV_TYPE_FILE_SIZE, sizeof(long));
    set_tlv_value_long(&sub_tlv_file_size, file_size);

    set_next_tlv(&sub_tlv_file_name, &sub_tlv_file_size);
    set_sub_tlv_list(&tlv_header, &sub_tlv_file_name);
    if (!send_tlv_data(data->transmission_socket, &tlv_header)) {
        goto RELEASE_ON_ERROR;
    }
    tlv_release_tlvs();
    free(filename);
    filename = NULL;
    return true;

RELEASE_ON_ERROR:
    tlv_release_tlvs();
    free(filename);
    filename = NULL;
    return false;
}

/**
 * @brief Sends file content and digest.
 *
 * @param socket The socket to be used
 * @param fp The pointer to the opened file
 *
 * @return true if header information was sent successfully
 * @return false otherwise
 **/
bool send_file_content(sal_socket_t socket, FILE* fp) {
    static uint8_t buffer[TLV_MAX_VALUE_LENGTH] = {0};

    if (ferror(fp)) {
        return false;
    }

    SHA512_CTX sha512_ctx;
    SHA512_Init(&sha512_ctx);
    do {
        const size_t read_bytes = fread(buffer, 1, sizeof(buffer), fp);
        if (ferror(fp)) {
            goto RELEASE_ON_ERROR;
        }
        if (read_bytes > 0) {
            tlv_t tlv_content = new_tlv(TLV_TYPE_FILE_CONTENT, read_bytes);
            set_tlv_value_raw(&tlv_content, buffer);
            if (!send_tlv_data(socket, &tlv_content)) {
                goto RELEASE_ON_ERROR;
            }
            SHA512_Update(&sha512_ctx, buffer, read_bytes);
            tlv_release_tlvs();
        }
    } while (!feof(fp));
    SHA512_Final(buffer, &sha512_ctx);
    tlv_t tlv_sha512 = new_tlv(TLV_TYPE_CHECKSUM_SHA512, SHA512_DIGEST_LENGTH);
    set_tlv_value_raw(&tlv_sha512, buffer);
    if (!send_tlv_data(socket, &tlv_sha512)) {
        return false;
    }
    tlv_release_tlvs();

    return check_reply(socket);

RELEASE_ON_ERROR:
    tlv_release_tlvs();
    return false;
}

/**
 * @brief Establishes a connection and sends a file through it.
 *
 * @param data The client internal data
 *
 * @return true if file content was received, written and validated successfully
 * @return false otherwise
 **/
void send_file(client_data* data) {
    FILE* fp = NULL;
    if ((fp = fopen(data->path, "rb")) == NULL) {
        print_error("Open file failed");
        return;
    }

    if ((data->transmission_socket = sal_create_socket()) == NULL) {
        goto CLOSE_FILE;
    }

    if (sal_connect(data->transmission_socket, &data->server_addr) != SAL_OK) {
        goto DESTROY_SOCKET;
    }

    print_msg("Sending file \"%s\" containing %ld bytes...", data->path, get_filesize(fp));
    fflush(stdout);
    if (!send_header(data, fp)) {
        goto CLOSE_SOCKET;
    }
    if (!send_file_content(data->transmission_socket, fp)) {
        goto CLOSE_SOCKET;
    }
    print_msg(" done\n");

    sal_close(data->transmission_socket);
    sal_destroy_socket(data->transmission_socket);
    data->transmission_socket = NULL;
    fclose(fp);
    fp = NULL;
    return;

CLOSE_SOCKET:
    print_msg(" error\n");
    sal_close(data->transmission_socket);
DESTROY_SOCKET:
    sal_destroy_socket(data->transmission_socket);
    data->transmission_socket = NULL;
CLOSE_FILE:
    fclose(fp);
    fp = NULL;
}

/**
 * @brief Checks server reply to ensure that file was received successfully.
 *
 * @param socket The used socket
 *
 * @return No return
 **/
bool check_reply(sal_socket_t socket) {
    tlv_t tlv = {0};
    if (!receive_tlv_data(socket, &tlv)) {
        print_warning("Reply check failed");
    }
    bool ack = get_tlv_type(&tlv) == TLV_TYPE_ACK;
    tlv_release_tlvs();
    return ack;
}

/**
 * @brief Parses input arguments and validate them.
 *
 * @param argc The number of arguments
 * @param argv The arguments values
 * @param[out] data The client internal data
 *
 * @return true if given arguments are valid
 * @return false otherwise
 **/
bool parse_input(const int argc, const char** argv, client_data* data) {
    if (argc != 4) {
        return false;
    }

    const char* path = argv[1];
    switch (sal_is_file_readable(path)) {
    case SAL_FILE_NOT_FOUND:
        set_error_description("%s", path);
        print_error("File not found");
        return false;
        break;
    case SAL_FILE_NOT_READABLE:
        set_error_description("%s", path);
        print_error("File is not readable");
        return false;
        break;
    default:
        break;
    }

    struct in_addr server_ip_addr = {0};
    if (inet_aton(argv[2], &server_ip_addr) == 0) {
        set_error_description("%s", argv[2]);
        print_error("Invalid destination IP");
        return false;
    }

    const int server_port = atoi(argv[3]);
    if ((server_port <= 0) || (server_port > 65535)) {
        set_error_description("%d", server_port);
        print_error("Invalid destination port");
        return false;
    }

    data->path = strdup(path);
    data->server_addr.sin_addr = server_ip_addr;
    data->server_addr.sin_port = htons(server_port);
    data->server_addr.sin_family = AF_INET;

    return true;
}

/**
 * @brief Releases client internal data.
 *
 * @param data The client internal data
 *
 * @return true if given arguments are valid
 * @return false otherwise
 **/
void release_client_data(client_data* data) {
    free(data->path);
    data->path = NULL;
    sal_destroy_socket(data->transmission_socket);
    data->transmission_socket = NULL;
}
