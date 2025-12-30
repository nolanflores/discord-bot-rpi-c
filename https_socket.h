#ifndef SOCKET_H
#define SOCKET_H

#include <openssl/ssl.h>

struct https_socket{
    const char* hostname;
    int socket_fd;
    SSL* ssl;
};

int https_ctx_init();

int https_tcp_connect(struct https_socket* sock, const char* hostname, const char* port);

int https_tls_connect(struct https_socket* sock);

char* https_send(struct https_socket* sock, const char* request);

void https_close(struct https_socket* sock);

void https_ctx_free();

#endif