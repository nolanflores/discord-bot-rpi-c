#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include "core/https_socket.h"

/*
 * Structure representing a WebSocket connection.
 *
 * This structure is a wrapper for the underlying https_socket,
 * and exists to prevent passing an upgraded socket to https functions.
 */
struct websocket{
    struct https_socket https_sock;
};

/*
* Represents a received WebSocket message.
* 
* opcode: The WebSocket opcode (e.g., 1 for text, 2 for binary).
* payload: The message payload as a null-terminated string.
* payload_len: The length of the payload in bytes.
*/
struct ws_message{
    int opcode;
    char* payload;
    size_t payload_len;
};

/*
 * Establishes a TCP/TLS socket connection.
 * Then performs the WebSocket upgrade handshake.
 * Closes the TCP socket on failure.
 * 
 * This function performs the DNS lookup internally.
 * 
 * Returns 0 on success, 1 on failure.
 */
int ws_connect(struct websocket* ws, const char* hostname, const char* port);

/*
 * Sends a text websocket frame (opcode 1) with the given message.
 * Accepts messages up to 4096 bytes in length, including the null terminator.
 * This is a self imposed limit, as the WebSocket protocol supports much larger messages.
 * However, the discord gateway has a single frame limit of 4096 bytes.
 *
 * Returns 0 on success, 1 on failure.
 */
int ws_send_text(struct websocket* ws, const char* message);

/*
 * Receives a websocket frame and returns the payload as a null-terminated string.
 * The caller is responsible for freeing the returned string.
 * 
 * Returns NULL if a close frame is received, or if reading fails.
 */
struct ws_message* ws_receive(struct websocket* ws);

/*
 * Sends a WebSocket close frame (opcode 8) to the server.
 * Closes the TLS connection and TCP socket.
 */
void ws_close(struct websocket* ws);

/*
 * Frees a ws_message structure.
 * Also frees the member payload.
 */
void ws_free_message(struct ws_message* msg);

#endif