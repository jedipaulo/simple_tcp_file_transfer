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
        "Usage: %s <file path> <destination IP address> <destination port>\n",
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

void send_file(int sockfd, const char* path) {
    char buf[MSG_BUFFER_LEN] = {0};

    char* filename = strrchr(path, '/') + 1;
    if (strlen(filename) == 0) {
        print_error("Invalid filename");
        return;
    }

    buf[0] = strlen(filename);
    if (send(sockfd, buf, 1, 0) < 0) {
        print_error("Send message failed");
        goto E1;
    }

    if (send(sockfd, filename, strlen(filename), 0) < 0) {
        print_error("Send message failed");
        return;
    }

    FILE* fp = fopen(path, "rb");
    fseek(fp, 0, SEEK_END); // seek to end of file
    unsigned long long int file_size = ftell(fp); // get current file pointer
    fseek(fp, 0, SEEK_SET); // seek back to beginning of file

    for (int i = 0; i < sizeof(file_size); ++i) {
        buf[i] = file_size >> ((sizeof(file_size) - i - 1) * 8);
    }
    if (send(sockfd, buf, sizeof(file_size), 0) < 0) {
        print_error("Send message failed");
        goto E1;
    }

    printf("Sending file \"%s\" (%s) containing %llu bytes...", filename, path, file_size);

    unsigned long long int bytes_transmitted = 0;
    int read_bytes = 0;
    while (bytes_transmitted < file_size) {
        read_bytes = file_size - bytes_transmitted;
        read_bytes = read_bytes < MSG_BUFFER_LEN ? read_bytes : MSG_BUFFER_LEN;
        fread(buf, read_bytes, 1, fp);
        // Send the message to server
        if (send(sockfd, buf, read_bytes, 0) < 0) {
            print_error("Send message failed");
            goto E1;
        }
        bytes_transmitted += read_bytes;
    }
    printf(" done\n");

E1:
    fclose(fp);
    return;
}

int main(const int argc, const char** argv) {
    if (argc != 4) {
        print_usage(argv[0]);
        exit(1);
    }

    const char* path = argv[1];
    switch (sal_file_exists(path)) {
    case SAL_FILE_NOT_FOUND:
        print_error_and_exit("File not found");
        break;
    case SAL_FILE_NOT_READABLE:
        print_error_and_exit("File is not readable");
        break;
    default:
        break;
    }

    struct in_addr server_ip_addr = {0};
    if (inet_aton(argv[2], &server_ip_addr) == 0) {
        print_error_and_exit("Invalid destination IP");
    }

    const int server_port = atoi(argv[3]);
    if ((server_port <= 0) || (server_port > 65535)) {
        print_error_and_exit("Invalid destination port");
    }

    printf("Port: %d\n", server_port);
    printf(" Dir: %s\n", path);
    printf("  IP: %s\n", inet_ntoa(server_ip_addr));

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        print_error_and_exit("Socket creation failed");
    }

    struct sockaddr_in server_addr;
    server_addr.sin_addr = server_ip_addr;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_family = AF_INET;

    // Send connection request to server:
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
        print_error("Socket bind failed\n");
        goto E1;
    }

    send_file(sockfd, path);

E1:
    close(sockfd);
    return 1;
}
