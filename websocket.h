#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include <stdlib.h>
#include <openssl/ssl.h>
#include <pthread.h>

struct websocket{
    int socket_fd;
    SSL_CTX* ctx;
    SSL* ssl;
    pthread_t heartbeat_thread;
    pthread_mutex_t send_mutex;
    int heartbeat_interval_ms;
};

struct ws_message{
    int opcode;
    char* payload;
    size_t payload_len;
};

/*
 * Establishes a TCP connection to the WebSocket server.
 * Returns 0 on success, 1 on failure.
 */
int ws_tcp_connect(struct websocket* ws);

/*
 * Establishes a TLS connection over the existing TCP connection.
 * Closes the TCP socket on failure.
 * Returns 0 on success, 1 on failure.
 */
int ws_tls_connect(struct websocket* ws);

/*
 * Performs the WebSocket handshake over the established TLS connection.
 * Closes the TLS connection and TCP socket on failure.
 * Returns 0 on success, 1 on failure.
 */
int ws_handshake(struct websocket* ws);

/*
 * Send a text websocket frame (opcode 1) with the given message.
 * Returns 0 on success, 1 on failure.
 */
int ws_send_text(struct websocket* ws, const char* message);

int ws_start_heartbeat(struct websocket* ws, int interval_ms);

/*
 * Receives a websocket frame and returns the payload as a null-terminated string.
 * The caller is responsible for freeing the returned string.
 * Returns NULL on failure or if a close frame is received.
 */
struct ws_message* ws_receive(struct websocket* ws);

/*
 * Closes the TLS connection and TCP socket.
 */
void ws_close(struct websocket* ws);

/*
 * Frees a ws_message structure.
 * Also frees member payload.
 */
void ws_free_message(struct ws_message* msg);

#endif