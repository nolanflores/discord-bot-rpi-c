#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200112L

#include "core/https_socket.h"

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// Global SSL context used for all https_socket connections.
static SSL_CTX* ssl_ctx = NULL;

/*
 * Static helper function for performing an IPv4 DNS lookup for the given hostname and port.
 * 
 * Returns a pointer to a linked list of addrinfo structures on success,
 * or NULL on failure.
 * The returned addrinfo list must be freed with freeaddrinfo when no longer needed.
 */
static struct addrinfo* https_dns_lookup(const char* hostname, const char* port){
    struct addrinfo* addr_list = NULL;
    struct addrinfo hints = {0};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;
    hints.ai_protocol = 0;
    if(getaddrinfo(hostname, port, &hints, &addr_list) == 0)
        return addr_list;
    fputs("Failed to get address info\n", stderr);
    return NULL;
}

/*
 * Static helper function for establishing a TCP connection
 * Calls the DNS lookup for the given hostname and port.
 * Then connects the socket and frees the addrinfo list.
 * Closes the socket, frees the hostname and port on fail.
 * 
 * Returns 0 on success, 1 on failure.
 */
static int https_tcp_connect(struct https_socket* sock, const char* hostname, const char* port){
    struct addrinfo* addr_list = https_dns_lookup(hostname, port);
    if(addr_list == NULL)
        return 1;
    sock->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(sock->socket_fd == -1){
        fputs("Failed to create socket\n", stderr);
        return 1;
    }
    sock->hostname = strdup(addr_list->ai_canonname);
    sock->port = strdup(port);
    struct addrinfo* ptr = addr_list;
    while(ptr != NULL){
        int connection_result = connect(sock->socket_fd, ptr->ai_addr, ptr->ai_addrlen);
        if(connection_result == 0)
            return 0;
        ptr = ptr->ai_next;
    }
    fputs("Failed to connect to address\n", stderr);
    close(sock->socket_fd);
    free(sock->hostname);
    free(sock->port);
    return 1;
}

/*
 * Static helper function for establishing a TLS connection over an existing TCP socket.
 * Closes the TCP socket on failure.
 * 
 * Returns 0 on success, 1 on failure.
 */
static int https_tls_connect(struct https_socket* sock){
    sock->ssl = SSL_new(ssl_ctx);
    if(!sock->ssl){
        fputs("Failed to create SSL structure\n", stderr);
        close(sock->socket_fd);
        return 1;
    }
    SSL_set_fd(sock->ssl, sock->socket_fd);
    if(sock->hostname)
        SSL_set_tlsext_host_name(sock->ssl, sock->hostname);
    sock->session = NULL;
    SSL_set_session(sock->ssl, sock->session);
    if(SSL_connect(sock->ssl) != 1){
        fputs("SSL connection failed\n", stderr);
        SSL_free(sock->ssl);
        close(sock->socket_fd);
        return 1;
    }
    SSL_SESSION_free(sock->session);
    sock->session = SSL_get1_session(sock->ssl);
    return 0;
}


static int https_reconnect(struct https_socket* sock){
    SSL_shutdown(sock->ssl);
    SSL_free(sock->ssl);
    close(sock->socket_fd);
    char* old_hostname = sock->hostname;
    char* old_port = sock->port;
    if(https_tcp_connect(sock, old_hostname, old_port)){
        fputs("Failed to reconnect TCP socket\n", stderr);
        return 1;
    }
    if(https_tls_connect(sock)){
        fputs("Failed to reconnect TLS socket\n", stderr);
        return 1;
    }
    free(old_hostname);
    free(old_port);
    return 0;
}


int https_ctx_init(){
    if(ssl_ctx)
        return 0;
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    ssl_ctx = SSL_CTX_new(TLS_client_method());
    if(!ssl_ctx){
        fputs("Failed to initialize SSL context\n", stderr);
        return 1;
    }
    SSL_CTX_set_session_cache_mode(ssl_ctx, SSL_SESS_CACHE_CLIENT);
    return 0;
}



int https_connect(struct https_socket* sock, const char* hostname, const char* port){
    if(https_tcp_connect(sock, hostname, port))
        return 1;
    if(https_tls_connect(sock))
        return 1;
    return 0;
}



/*
 * Read HTTP headers from the SSL connection into the passed buffer.
 * Updates buffer_size to the number of bytes read.
 * 
 * Returns a pointer to the start of the body in the buffer on success,
 * or returns NULL on failure.
 */
static char* https_read_headers(SSL* ssl, char* buffer, size_t* buffer_size, size_t buffer_capacity){
    char chunk[8192];
    char* current = buffer;

    while(1){
        if(*buffer_size + 8192 > buffer_capacity){
            fputs("Failed to read response: Header too large\n", stderr);
            return NULL;
        }
        int bytes_read = SSL_read(ssl, chunk, 8191);
        if(bytes_read <= 0){
            fputs("Failed to read headers\n", stderr);
            return NULL;
        }
        *buffer_size += bytes_read;
        memcpy(current, chunk, bytes_read);
        current += bytes_read;
        *current = '\0';

        char* search_start;
        if(current - bytes_read >= buffer + 3)
            search_start = current - bytes_read - 3;
        else
            search_start = buffer;
        char* header_end = strstr(search_start, "\r\n\r\n");
        if(header_end){
            return header_end + 4;
        }
    }
}

/*
 * Reads the chunks of a chunked transfer encoded response.
 * Copies the contents of the chunks into a heap allocated string.
 * This should only be called after the headers have been read.
 * 
 * Returns the allocated string on success, NULL on failure
 */
static char* https_read_chunked(SSL* ssl, char* buffer, size_t* buffer_size, size_t buffer_capacity, char* body_start){
    while(1){
        if(memcmp(buffer + *buffer_size - 5, "0\r\n\r\n", 5) == 0)
            break;
        if(*buffer_size + 8192 >= buffer_capacity){
            fputs("Chunked response too large\n", stderr);
            return NULL;
        }
        int b_r = SSL_read(ssl, buffer + *buffer_size, 8192);
        if(b_r <= 0){
            fputs("Failed to read chunked response\n", stderr);
            return NULL;
        }
        *buffer_size += b_r;
        buffer[*buffer_size] = '\0';
    }
    
    char* response = (char*)malloc(*buffer_size - (body_start - buffer) + 1);
    if(!response){
        fputs("Failed to allocate memory for response\n", stderr);
        return NULL;
    }
    
    char* curr = body_start;
    size_t r_i = 0;
    
    while(1){
        size_t chunk_size = (size_t)strtoul(curr, NULL, 16);
        if(chunk_size == 0){
            response[r_i] = '\0';
            break;
        }
        
        char* chunk_start = strstr(curr, "\r\n");
        if(!chunk_start){
            fputs("Malformed chunked encoding\n", stderr);
            free(response);
            return NULL;
        }
        chunk_start += 2;
        
        memcpy(response + r_i, chunk_start, chunk_size);
        r_i += chunk_size;
        curr = chunk_start + chunk_size + 2;
    }
    
    return response;
}

/*
 * Read a response body with a known Content-Length from the SSL connection.
 *
 * Returns heap allocated string on success, NULL on failure
 */
static char* https_read_content_length(SSL* ssl, char* buffer, size_t* buffer_size, size_t buffer_capacity, char* body_start, size_t content_length){
    size_t received_body_len = *buffer_size - (body_start - buffer);
    
    while(received_body_len < content_length){
        if(*buffer_size + 8192 >= buffer_capacity){
            fputs("Response body too large\n", stderr);
            return NULL;
        }
        int b_r = SSL_read(ssl, buffer + *buffer_size, 8192);
        if(b_r <= 0){
            fputs("Failed to read response body\n", stderr);
            return NULL;
        }
        *buffer_size += b_r;
        received_body_len += b_r;
        buffer[*buffer_size] = '\0';
    }
    
    char* response = (char*)malloc(content_length + 1);
    if(!response){
        fputs("Failed to allocate memory for response\n", stderr);
        return NULL;
    }
    memcpy(response, body_start, content_length);
    response[content_length] = '\0';
    return response;
}

char* https_send(struct https_socket* sock, const char* data){
    if(SSL_write(sock->ssl, data, strlen(data)) <= 0){
        fputs("Failed to send request, attempting to reconnect\n", stderr);
        if(https_reconnect(sock)){
            fputs("Reconnection failed\n", stderr);
            return NULL;
        }
        if(SSL_write(sock->ssl, data, strlen(data)) <= 0){
            fputs("Failed to send request after reconnecting\n", stderr);
            return NULL;
        }
        fputs("Reconnected successfully\n", stderr);
    }
    
    char buffer[64 * 8192];
    size_t buffer_size = 0;
    char* header_end = https_read_headers(sock->ssl, buffer, &buffer_size, sizeof(buffer));
    if(!header_end){
        return NULL;
    }
    
    //chunked encoding
    if(strcasestr(buffer, "Transfer-Encoding: chunked\r\n")){
        return https_read_chunked(sock->ssl, buffer, &buffer_size, sizeof(buffer), header_end);
    }
    
    //Content-Length
    char* content_length_str = strcasestr(buffer, "Content-Length:");
    if(content_length_str){
        if(content_length_str > header_end){
            fputs("Error, Content-Length after header end\n", stderr);
            return NULL;
        }
        content_length_str += 15;
        size_t content_length = (size_t)strtoul(content_length_str, NULL, 10);
        return https_read_content_length(sock->ssl, buffer, &buffer_size, sizeof(buffer), header_end, content_length);
    }
    
    fputs("Response has neither Content-Length nor chunked encoding\n", stderr);
    return NULL;
}



void https_close(struct https_socket* sock){
    if(sock->ssl){
        SSL_shutdown(sock->ssl);
        SSL_free(sock->ssl);
    }
    if(sock->socket_fd != -1){
        close(sock->socket_fd);
    }
    if(sock->session){
        SSL_SESSION_free(sock->session);
        sock->session = NULL;
    }
    free(sock->hostname);
    free(sock->port);
    sock->hostname = NULL;
    sock->port = NULL;
}



void https_ctx_free(){
    if(ssl_ctx){
        SSL_CTX_free(ssl_ctx);
        ssl_ctx = NULL;
    }
}