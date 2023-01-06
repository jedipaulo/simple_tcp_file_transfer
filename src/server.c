#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h> // for close
#include <string.h>

#include "sal.h"

#define NUMBER_OF_CONNECTIONS 1
#define MSG_BUFFER_LEN 1024
#define MAX_PATH_LEN 1024

void print_usage(const char* app_name) {
    fprintf(
        stderr,
        "Usage: %s <storage directory> <listening IP address> <listening port>\n",
        app_name
    );
    exit(1);
}

void print_error_and_exit(const char *msg) {
    fprintf(stderr, "[Error] %s\n", msg);
    exit(1);
}

void print_error(const char *msg) {
    fprintf(stderr, "[Error] %s\n", msg);
}

void receiving_loop(int sockfd, const char* storage_dir) {
    /*
     * Accept a connection.
     */
    int connfd = -1;
    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);
    if ((connfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_len)) == -1) {
        print_error("Accepted failed");
        return;
    }

    /*
     * Receive the message on the newly connected socket.
     */
    char buf[MSG_BUFFER_LEN] = {0};
    int bytes_received = 0;
    if (bytes_received = recv(connfd, buf, 1, 0) <= 0) {
        print_error("Receiving failed");
        goto E2;
    }
    int filename_len = buf[0];
    if (filename_len == 0) {
        print_error("Invalid filename size");
        goto E2;
    }

    char* filename = malloc(filename_len + 1);
    int offset = 0;
    while (offset < filename_len) {
        if ((bytes_received = recv(connfd, buf, filename_len - offset, 0)) <= 0) {
            print_error("Receiving failed");
            goto E3;
        }
        memcpy(&filename[offset], buf, bytes_received);
        offset += bytes_received;
    }
    filename[filename_len] = '\0';

    unsigned long long int file_size = 0;
    offset = 0;
    while (offset < sizeof(file_size)) {
        if ((bytes_received = recv(connfd, buf, sizeof(file_size) - offset, 0)) <= 0) {
            print_error("Receiving failed");
            goto E3;
        }
        for (int i = 0; i < bytes_received; ++i) {
            file_size += (unsigned long long int)(buf[i]) << ((sizeof(file_size) - offset - i - 1) * 8);
        }
        offset += bytes_received;
    }
    char file_path[MAX_PATH_LEN] = {0};
    strcpy(file_path, storage_dir);
    strcat(file_path, "/"),
    strcat(file_path, filename),

    printf("Receiving file \"%s\" (%s) containing %llu bytes...", filename, file_path, file_size);
    fflush(stdout);
    FILE* fp = fopen(file_path, "w+b");
    if (fp == NULL) {
        print_error("Opening file failed");
        goto E3;
    }
    unsigned long long int remaining_bytes = file_size;
    while (remaining_bytes) {
        int requested_bytes = remaining_bytes > MSG_BUFFER_LEN ? MSG_BUFFER_LEN : remaining_bytes;
        if ((bytes_received = recv(connfd, buf, requested_bytes, 0)) <= 0) {
            print_error("Receiving failed");
            goto E4;
        }
        if (fwrite(buf, bytes_received, 1, fp) != 1) {
            print_error("Writing failed");
            goto E4;
        }
        fflush(fp);
        remaining_bytes -= bytes_received;
    }
    fclose(fp);
    printf(" done\n");
    return;

E4:
    fclose(fp);
E3:
    free(filename);
E2:
    close(connfd);
}

int main(const int argc, const char** argv) {
    if (argc != 4) {
        print_usage(argv[0]);
        exit(1);
    }

    const char* storage_dir = argv[1];
    switch (sal_dir_exists(storage_dir)) {
    case SAL_DIR_NOT_FOUND:
        print_error_and_exit("Storage directory not found");
        break;
    case SAL_DIR_NOT_WRITABLE:
        print_error_and_exit("Storage directory is not writable");
        break;
    default:
        break;
    }

    const int server_port = atoi(argv[3]);
    if ((server_port <= 0) || (server_port > 65535)) {
        print_error_and_exit("Invalid listening port");
    }

    struct in_addr server_ip_addr = {0};
    if (inet_aton(argv[2], &server_ip_addr) == 0) {
        print_error_and_exit("Invalid listening IP");
    }

    printf("Port: %d\n", server_port);
    printf(" Dir: %s\n", storage_dir);
    printf("  IP: %s\n", inet_ntoa(server_ip_addr));

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        print_error_and_exit("Socket creation failed");
    }

    struct sockaddr_in server_addr;
    server_addr.sin_addr = server_ip_addr;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_family = AF_INET;
    if ((bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr))) != 0) {
        print_error("Socket bind failed\n");
        goto E1;
    }

    /*
     * Listen for connections. Specify the backlog as 1.
     */
    if (listen(sockfd, NUMBER_OF_CONNECTIONS) != 0) {
        print_error("Listen failed");
        goto E1;
    }
    while (true) {
        receiving_loop(sockfd, storage_dir);
    }

E1:
    close(sockfd);
    return 1;
}
