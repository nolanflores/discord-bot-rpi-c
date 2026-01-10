#ifndef SOCKET_H
#define SOCKET_H

#include <openssl/ssl.h>

/*
 * Structure representing an HTTPS socket connection.
 * 
 * hostname: The canonical name of the connected server.
 * socket_fd: The file descriptor of the TCP socket.
 * ssl: The SSL structure for the TLS connection.
 */
struct https_socket{
    char* hostname;
    int socket_fd;
    SSL* ssl;
};

/*
 * Initializes the global SSL context.
 * This function must be called before any other https functions.
 * https_ctx_free should be called when the context is no longer needed.
 * 
 * Returns 0 on success, 1 on failure.
 */
int https_ctx_init();

/*
 * Performs an IPv4 DNS lookup for the given hostname and port.
 * 
 * Returns a pointer to a linked list of addrinfo structures on success,
 * or NULL on failure.
 * The returned addrinfo list must be freed with freeaddrinfo when no longer needed.
 */
struct addrinfo* https_dns_lookup(const char* hostname, const char* port);

/*
 * Establishes a TCP/TLS socket connection.
 * Closes the socket on failure.
 * Alloctes and sets the hostname field of the https_socket.
 * 
 * This function takes an addrinfo list obtained from https_dns_lookup,
 * allowing the caller to control the lifetime of the addrinfo list.
 * 
 * Returns 0 on success, 1 on failure.
 */
int https_connect_addrinfo(struct https_socket* sock, struct addrinfo* addr_list);

/*
 * Establishes a TCP/TLS socket connection.
 * Closes the TCP socket on failure.
 * Allocates and sets the hostname field of the https_socket.
 * 
 * This function performs the DNS lookup internally.
 * 
 * Returns 0 on success, 1 on failure.
 */
int https_connect(struct https_socket* sock, const char* hostname, const char* port);

/*
 * Sends an HTTP request over the given https_socket.
 * 
 * Returns the response body as a null-terminated string on success,
 * or NULL on failure.
 */
char* https_send(struct https_socket* sock, const char* request);

/*
 * Closes the TLS connection and TCP socket.
 * Frees the hostname string.
 */
void https_close(struct https_socket* sock);

/*
 * Frees the global SSL context.
 * This function should be called when the SSL context is no longer needed.
 */
void https_ctx_free();

#endif