#include "websocket.h"
#include <openssl/rand.h>

/*
 * Static helper function for connecting a websocket.
 * Performs the WebSocket handshake over the established TLS connection.
 * Closes the TLS connection and TCP socket on failure.
 * 
 * Returns 0 on success, 1 on failure.
 */
static int ws_handshake(struct websocket* ws){
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
        fprintf(stderr, "Failed to send websocket upgrade request\n");
        return 1;
    }
    char response[8192];
    int bytes_read = SSL_read(ws->https_sock.ssl, response, 8191);
    if(bytes_read <= 0){
        fprintf(stderr, "Failed to read websocket upgrade response\n");
        return 1;
    }
    response[bytes_read] = '\0';
    if(strncmp(response, "HTTP/1.1 101", 12) != 0){
        fprintf(stderr, "WebSocket upgrade failed\n");
        return 1;
    }
    return 0;
}



int ws_connect_addrinfo(struct websocket* ws, struct addrinfo* addr_list){
    /*Reminder that https_connect_addrinfo does not free the addrinfo list.
    That's really the reason this function exists, to allow the caller to connect
    multiple sockets without needlessly repeating DNS lookups.*/
    if(https_connect_addrinfo(&ws->https_sock, addr_list)){
        fprintf(stderr, "Websocket TCP connection failed\n");
        return 1;
    }
    if(ws_handshake(ws)){//static helper function
        https_close(&ws->https_sock);
        return 1;
    }
    return 0;
}



int ws_connect(struct websocket* ws, const char* hostname, const char* port){
    /*Connects the https_socket to the given hostname and port, then performs WebSocket handshake.
    This is the standard connect function, where DNS lookup is performed internally and freed.*/
    if(https_connect(&ws->https_sock, hostname, port)){
        fprintf(stderr, "Websocket TCP connection failed\n");
        return 1;
    }
    if(ws_handshake(ws)){//
        https_close(&ws->https_sock);
        return 1;
    }
    return 0;
}

int ws_send_text(struct websocket* ws, const char* message){
    size_t payload_len = strlen(message);
    if(payload_len >= 4096){
        fprintf(stderr, "Payload too large\n");
        return 1;
    }
    size_t offset = 0;
    if(payload_len >= 126)
        offset = 2;
    size_t frame_size = 6 + payload_len + offset;
    unsigned char* frame = (unsigned char*)malloc(frame_size);
    if(!frame){
        fprintf(stderr, "Failed to allocate memory for frame\n");
        return 1;
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
        fprintf(stderr, "Failed to send WebSocket frame\n");
        return 1;
    }
    return 0;
}

struct ws_message* ws_receive(struct websocket* ws){
    struct ws_message* msg = (struct ws_message*)malloc(sizeof(struct ws_message));
    if(!msg){
        fprintf(stderr, "Failed to allocate memory for ws_message\n");
        return NULL;
    }
    msg->opcode = 0;
    char* payloads[100] = {0};
    size_t payload_lengths[100];
    int payload_count = 0;
    unsigned char header[2];
    do{
        SSL_read(ws->https_sock.ssl, header, 2);
        if(!msg->opcode){
            msg->opcode = header[0] & 0x0F;
            if(msg->opcode == 8){
                fprintf(stderr, "Received close frame\n");
                msg->payload = NULL;
                msg->payload_len = 0;
                return msg;
            }
        }
        payload_lengths[payload_count] = header[1] & 0x7F;
        if(payload_lengths[payload_count] == 126){
            unsigned char ext[2];
            SSL_read(ws->https_sock.ssl, ext, 2);
            payload_lengths[payload_count] = ((size_t)ext[0] << 8) | (size_t)ext[1];
        }
        payloads[payload_count] = (char*)malloc(payload_lengths[payload_count]+1);
        int bytes_read = SSL_read(ws->https_sock.ssl, payloads[payload_count], payload_lengths[payload_count]);
        if(bytes_read <= 0){
            fprintf(stderr, "Failed to read WebSocket payload\n");
            ws_free_message(msg);
            return NULL;
        }
        payload_count++;
        if(payload_count >= 100){
            fprintf(stderr, "Too many fragmented frames\n");
            ws_free_message(msg);
            return NULL;
        }
    }while(!(header[0] & 0x80));
    if(payload_count == 1){
        msg->payload = payloads[0];
        msg->payload[payload_lengths[0]] = '\0';
        msg->payload_len = payload_lengths[0];
    }else{
        size_t total_len = 0;
        for(int i = 0; i < payload_count; i++){
            total_len += payload_lengths[i];
        }
        msg->payload = (char*)malloc(total_len + 1);
        if(!msg->payload){
            fprintf(stderr, "Failed to allocate memory for combined payload\n");
            for(int i = 0; i < payload_count; i++)
                free(payloads[i]);
            ws_free_message(msg);
            return NULL;
        }
        size_t offset = 0;
        for(int i = 0; i < payload_count; i++){
            memcpy(msg->payload + offset, payloads[i], payload_lengths[i]);
            offset += payload_lengths[i];
            free(payloads[i]);
        }
        msg->payload[total_len] = '\0';
        msg->payload_len = total_len;
    }
    return msg;
}

void ws_free_message(struct ws_message* msg){
    if(msg)
        free(msg->payload);
    free(msg);
}

void ws_close(struct websocket* ws){
    if(ws->https_sock.ssl){
        unsigned char close_frame[6] = {0x88, 0x80};
        RAND_bytes(&close_frame[2], 4);
        SSL_write(ws->https_sock.ssl, close_frame, 6);
    }
    https_close(&ws->https_sock);
}