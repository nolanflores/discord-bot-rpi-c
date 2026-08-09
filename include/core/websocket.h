#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include "core/https_socket.h"

/*
 * Structure representing a WebSocket connection.
 *
 * This structure is a wrapper for the underlying https_socket,
 * and exists to prevent passing an upgraded socket to https functions.
 */
typedef struct websocket{
    https_socket https_sock;
} websocket;

/*
 * Estableshes a TCP/TLS socket connection.
 * Then performs the WebSocket upgrade handshake.
 * Closes the TCP socket on failure.
 * 
 * This function performs the DNS lookup internally.
 * 
 * Returns 0 on success, -1 on failure.
 */
int ws_connect(websocket* ws, const char* hostname, const char* port);

/*
 * Sends a text websocket frame (opcode 1) with a passed payload.
 * Accepts payloads up to 4096 bytes in length, including the null terminator.
 *
 * Returns 0 on success, -1 on failure.
 */
int ws_send(websocket* ws, const char* message);

/*
 * Receives the opcode and payload of a WebSocket frame.
 * 
 * Returns the opcode if a valid frame is received, or -1 if reading fails.
 * The payload is written to the provided buffer, up to buffer_size bytes.
 * If the payload is larger than buffer_size, it will be treated as a failure.
 */
int ws_receive(websocket* ws, char* buffer, size_t buffer_size);

/*
 * Sends a WebSocket close frame (opcode 8) to the server.
 * Closes the TLS connection and TCP socket.
 */
void ws_close(websocket* ws);

#endif