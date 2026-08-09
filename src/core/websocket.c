#include "core/websocket.h"
#include <openssl/rand.h>

/*
 * Static helper function for ws_connect().
 * Performs the WebSocket handshake over the established TLS connection.
 * Closes the TLS connection and TCP socket on failure.
 * 
 * Returns 0 on success, -1 on failure.
 */
static inline int ws_handshake(websocket* ws){
    unsigned char rand_key[16];
    RAND_bytes(rand_key, 16);
    char base64_key[64];
    int len = EVP_EncodeBlock((unsigned char*)base64_key, rand_key, 16);
    base64_key[len] = '\0';
    char request[512];
    snprintf(request, 512,
        "GET /ws HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: %s\r\n"
        "Sec-WebSocket-Version: 13\r\n\r\n",
        ws->https_sock.hostname, base64_key
    );
    if(0 >= SSL_write(ws->https_sock.ssl, request, strlen(request))){
        fputs("Failed to send websocket upgrade request\n", stderr);
        return -1;
    }
    char response[8192];
    int bytes_read = SSL_read(ws->https_sock.ssl, response, 8191);
    if(bytes_read <= 0){
        fputs("Failed to read websocket upgrade response\n", stderr);
        return -1;
    }
    response[bytes_read] = '\0';
    if(strncmp(response, "HTTP/1.1 101", 12) != 0){
        fputs("WebSocket upgrade failed\n", stderr);
        return -1;
    }
    return 0;
}

/* ============================================================================
 * Public Functions
 * ============================================================================
*/ 


int ws_connect(websocket* ws, const char* hostname, const char* port){
    if(https_connect(&ws->https_sock, hostname, port)){
        fputs("Websocket TCP connection failed\n", stderr);
        return -1;
    }
    if(ws_handshake(ws)){
        https_close(&ws->https_sock);
        return -1;
    }
    return 0;
}



int ws_send(websocket* ws, const char* message){
    size_t payload_len = strlen(message);
    if(payload_len >= 4096){
        fputs("Payload too large\n", stderr);
        return -1;
    }
    size_t offset = 0;
    if(payload_len >= 126)
        offset = 2;
    size_t frame_size = 6 + payload_len + offset;
    unsigned char* frame = (unsigned char*)malloc(frame_size);
    if(!frame){
        fputs("Failed to allocate memory for frame\n", stderr);
        return -1;
    }
    frame[0] = 0x81;
    if(payload_len < 126){
        frame[1] = 0x80 | (unsigned char)payload_len;
    }else{
        frame[1] = 0xFE;
        frame[2] = (unsigned char)((payload_len >> 8) & 0xFF);
        frame[3] = (unsigned char)(payload_len & 0xFF);
    }
    RAND_bytes(&frame[2+offset], 4);
    for(size_t i = 0; i < payload_len; i++){
        frame[6+offset+i] = message[i] ^ frame[2+offset+(i%4)];
    }
    int write_result = SSL_write(ws->https_sock.ssl, frame, frame_size);
    free(frame);
    if(write_result <= 0){
        fputs("Failed to send WebSocket frame\n", stderr);
        return -1;
    }
    return 0;
}



int ws_receive(websocket* ws, char* buffer, size_t buffer_size){
    int opcode = 0;
    size_t payload_len = 0;
    unsigned char header[2];
    do{
        SSL_read(ws->https_sock.ssl, header, 2);
        if(opcode == 0){
            opcode = header[0] & 0x0F;
            if(opcode == 8){
                fputs("Received close frame\n", stderr);
                return -1;
            }
        }
        int frame_len = header[1] & 0x7F;
        if(frame_len == 126){
            unsigned char ext[2];
            SSL_read(ws->https_sock.ssl, ext, 2);
            frame_len = ((size_t)ext[0] << 8) | (size_t)ext[1];
        }
        if(payload_len + frame_len >= buffer_size){
            fputs("Payload too large for buffer\n", stderr);
            return -1;
        }
        int bytes_read = 0;
        while(bytes_read < frame_len){
            int b_r = SSL_read(ws->https_sock.ssl, buffer + payload_len + bytes_read, frame_len - bytes_read);
            if(b_r <= 0){
                fputs("Failed to read WebSocket payload\n", stderr);
                return -1;
            }
            bytes_read += b_r;
        }
        payload_len += frame_len;
    }while(!(header[0] & 0x80));
    buffer[payload_len] = '\0';
    return payload_len;
}



void ws_close(websocket* ws){
    if(ws->https_sock.ssl){
        unsigned char close_frame[6] = {0x88, 0x80};
        RAND_bytes(close_frame + 2, 4);
        SSL_write(ws->https_sock.ssl, close_frame, 6);
    }
    https_close(&ws->https_sock);
}