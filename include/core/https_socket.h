#ifndef SOCKET_H
#define SOCKET_H

#include <openssl/ssl.h>

/*
 * Structure representing an HTTPS socket connection.
 * 
 * hostname: The canonical name of the connected server.
 * port: The port number as a string.
 * socket_fd: The file descriptor of the TCP socket.
 * ssl: The SSL structure for the TLS connection.
 * session: The SSL session for session resumption.
 */
typedef struct https_socket{
    char* hostname;
    char* port;
    int socket_fd;
    SSL* ssl;
    SSL_SESSION* session;
} https_socket;

/*
 * Establishes a TCP/TLS socket connection.
 * Closes the TCP socket on failure.
 * Allocates and sets the hostname and port fields of the https_socket.
 * 
 * Returns 0 on success, -1 on failure.
 */
int https_connect(https_socket* sock, const char* hostname, const char* port);

/*
 * Sends an HTTP request over the given https_socket.
 * 
 * Returns the response body as a null-terminated string on success,
 * or NULL on failure.
 */
char* https_send(https_socket* sock, const char* request);

/*
 * Closes the TLS connection and TCP socket.
 * Frees the hostname string.
 */
void https_close(https_socket* sock);

#endif