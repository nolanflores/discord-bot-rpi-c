#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200112L

#include "https_socket.h"

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// Global SSL context used for all https_socket connections.
static SSL_CTX* ssl_ctx = NULL;

/*
 * Static helper function for establishing a TCP connection
 * Does not free the addrinfo list.
 * Sets the https_socket hostname to the canonical name of the first address in the list.
 * 
 * Returns 0 on success, 1 on failure.
 */
static int https_tcp_connect_addrinfo(struct https_socket* sock, struct addrinfo* addr_list){
    if(!addr_list){
        fprintf(stderr, "Address info is NULL\n");
        return 1;
    }
    sock->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(sock->socket_fd == -1){
        fprintf(stderr, "Failed to create socket\n");
        return 1;
    }
    struct addrinfo* ptr = addr_list;
    sock->hostname = strdup(ptr->ai_canonname);
    while(ptr != NULL){
        int connection_result = connect(sock->socket_fd, ptr->ai_addr, ptr->ai_addrlen);
        if(connection_result == 0)
            return 0;
        ptr = ptr->ai_next;
    }
    fprintf(stderr, "Failed to connect to address\n");
    close(sock->socket_fd);
    return 1;
}

/*
 * Static helper function for establishing a TCP connection
 * Performs the DNS lookup for the given hostname.
 * Then connects the socket and frees the addrinfo list.
 * 
 * Returns 0 on success, 1 on failure.
 */
static int https_tcp_connect(struct https_socket* sock, const char* hostname, const char* port){
    struct addrinfo* addr_list = https_dns_lookup(hostname, port);
    int connect_result = https_tcp_connect_addrinfo(sock, addr_list);
    freeaddrinfo(addr_list);
    return connect_result;
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
        fprintf(stderr, "Failed to create SSL structure\n");
        close(sock->socket_fd);
        return 1;
    }
    SSL_set_fd(sock->ssl, sock->socket_fd);
    if(SSL_connect(sock->ssl) != 1){
        fprintf(stderr, "SSL connection failed\n");
        SSL_free(sock->ssl);
        close(sock->socket_fd);
        return 1;
    }
    return 0;
}



int https_ctx_init(){
    if(ssl_ctx)
        return 0;
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    ssl_ctx = SSL_CTX_new(TLS_client_method());
    if(ssl_ctx)
        return 0;
    fprintf(stderr, "Failed to initialize SSL context\n");
    return 1;
}



struct addrinfo* https_dns_lookup(const char* hostname, const char* port){
    struct addrinfo* addr_list = NULL;
    struct addrinfo hints = {0};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;
    hints.ai_protocol = 0;
    int result = getaddrinfo(hostname, port, &hints, &addr_list);
    if(result != 0){
        fprintf(stderr, "Failed to get address info\n");
        return NULL;
    }
    return addr_list;
}



int https_connect_addrinfo(struct https_socket* sock, struct addrinfo* addr_list){
    if(https_tcp_connect_addrinfo(sock, addr_list))
        return 1;
    if(https_tls_connect(sock))
        return 1;
    return 0;
}



int https_connect(struct https_socket* sock, const char* hostname, const char* port){
    if(https_tcp_connect(sock, hostname, port))
        return 1;
    if(https_tls_connect(sock))
        return 1;
    return 0;
}



char* https_send(struct https_socket* sock, const char* data){
    /*
     * This function is ridiculously long and I barely understand what I have created here.
     * I do not think it is robust, and is probably susceptible to a buffer overflow.
     * I will have to rewrite it later.
     */
    if(SSL_write(sock->ssl, data, strlen(data)) <= 0){
        fprintf(stderr, "Failed to send request\n");
        return NULL;
    }
    //8KiB = 8192
    char buffer[64 * 8192];
    int buffer_size = 0;
    char* header_end = NULL;
    while(1){
        if(buffer_size + 8192 >= 524288){
            fprintf(stderr, "Response too large\n");
            return NULL;
        }
        int b_r = SSL_read(sock->ssl, buffer + buffer_size, 8192);
        if(b_r <= 0){
            fprintf(stderr, "Failed to read response\n");
            return NULL;
        }
        buffer[buffer_size + b_r] = '\0';
        header_end = strstr(buffer + buffer_size, "\r\n\r\n");
        buffer_size += b_r;
        if(header_end){
            header_end += 4;
            break;
        }
    }
    if(strstr(buffer, "Transfer-Encoding: chunked\r\n")){
        while(1){
            if(buffer_size + 8192 >= 524288){
                fprintf(stderr, "Response too large\n");
                return NULL;
            }
            int b_r = SSL_read(sock->ssl, buffer + buffer_size, 8192);
            if(b_r <= 0){
                fprintf(stderr, "Failed to read chunked response\n");
                return NULL;
            }
            buffer_size += b_r;
            buffer[buffer_size] = '\0';
            if(strcmp(buffer + buffer_size - 5, "0\r\n\r\n") == 0)
                break;
        }
        char* response = (char*)malloc(buffer_size - (header_end - buffer) + 1);
        char* curr = header_end;
        int r_i = 0;
        while(1){
            int jump = (int)strtol(curr, NULL, 16);
            if(jump == 0){
                response[r_i] = '\0';
                break;
            }
            char* chunk_start = strstr(curr, "\r\n")+2;
            memcpy(response + r_i, chunk_start, jump);
            r_i += jump;
            curr = chunk_start + jump + 2;
        }
        return response;
    }else{
        char* content_length_str = strstr(buffer, "Content-Length: ");
        if(content_length_str){
            if(content_length_str > header_end){
                fprintf(stderr, "Error, Content-Length after header end\n");
                return NULL;
            }
            content_length_str += 16;
            int content_length = (int)strtol(content_length_str, NULL, 10);
            size_t received_body_len = buffer_size - (header_end - buffer);
            while(received_body_len < (size_t)content_length){
                if(buffer_size + 8192 >= 524288){
                    fprintf(stderr, "Response too large\n");
                    return NULL;
                }
                int b_r = SSL_read(sock->ssl, buffer + buffer_size, 8192);
                if(b_r <= 0){
                    fprintf(stderr, "Failed to read response body\n");
                    return NULL;
                }
                buffer_size += b_r;
                received_body_len += b_r;
                buffer[buffer_size] = '\0';
            }
            char* response = (char*)malloc(content_length + 1);
            memcpy(response, header_end, content_length);
            response[content_length] = '\0';
            return response;
        }
    }
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
    free(sock->hostname);
    sock->hostname = NULL;
}



void https_ctx_free(){
    if(ssl_ctx){
        SSL_CTX_free(ssl_ctx);
        ssl_ctx = NULL;
    }
}