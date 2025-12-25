#define _POSIX_C_SOURCE 200112L
#include "websocket.h"
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <netdb.h>
#include <openssl/rand.h>
#include <time.h>


int ws_tcp_connect(struct websocket* ws){
    ws->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(ws->socket_fd == -1){
        fprintf(stderr, "Failed to create socket\n");
        return 1;
    }
    struct addrinfo* addr_list = NULL;
    struct addrinfo hints = {0};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    int result = getaddrinfo("gateway.discord.gg", "443", &hints, &addr_list);
    if(result != 0){
        fprintf(stderr, "getaddrinfo failed\n");
        return 1;
    }
    struct addrinfo* ptr = addr_list;
    while(ptr != NULL){
        int connection_result = connect(ws->socket_fd, ptr->ai_addr, ptr->ai_addrlen);
        if(connection_result == 0){
            freeaddrinfo(addr_list);
            pthread_mutex_init(&ws->send_mutex, NULL);
            return 0;
        }
        ptr = ptr->ai_next;
    }
    fprintf(stderr, "Failed to connect to any address\n");
    freeaddrinfo(addr_list);
    close(ws->socket_fd);
    return 1;
}

int ws_tls_connect(struct websocket* ws){
    ws->ctx = SSL_CTX_new(TLS_client_method());
    if(!ws->ctx){
        fprintf(stderr, "Failed to create SSL_CTX\n");
        close(ws->socket_fd);
        return 1;
    }
    ws->ssl = SSL_new(ws->ctx);
    if(!ws->ssl){
        fprintf(stderr, "Failed to create SSL structure\n");
        SSL_CTX_free(ws->ctx);
        close(ws->socket_fd);
        return 1;
    }
    SSL_set_fd(ws->ssl, ws->socket_fd);
    if(SSL_connect(ws->ssl) != 1){
        fprintf(stderr, "SSL connection failed\n");
        SSL_free(ws->ssl);
        SSL_CTX_free(ws->ctx);
        close(ws->socket_fd);
        return 1;
    }
    return 0;
}

int ws_handshake(struct websocket* ws){
    unsigned char rand_key[16];
    RAND_bytes(rand_key, 16);
    char base64_key[64];
    int len = EVP_EncodeBlock((unsigned char*)base64_key, rand_key, 16);
    base64_key[len] = '\0';
    char request[512];
    snprintf(request, 512,
        "GET /ws HTTP/1.1\r\n"
        "Host: gateway.discord.gg\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: %s\r\n"
        "Sec-WebSocket-Version: 13\r\n\r\n",
        base64_key
    );
    if(0 >= SSL_write(ws->ssl, request, strlen(request))){
        fprintf(stderr, "Failed to send WebSocket upgrade request\n");
        return 1;
    }

    char response[1024];
    int bytes_read = SSL_read(ws->ssl, response, sizeof(response) - 1);
    if(bytes_read <= 0){
        fprintf(stderr, "Failed to read response\n");
        return 1;
    }
    response[bytes_read] = '\0';
    
    if(strncmp(response, "HTTP/1.1 101", 12) != 0){
        fprintf(stderr, "WebSocket upgrade failed\n");
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
    pthread_mutex_lock(&ws->send_mutex);
    int write_result = SSL_write(ws->ssl, frame, frame_size);
    pthread_mutex_unlock(&ws->send_mutex);
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
        SSL_read(ws->ssl, header, 2);
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
            SSL_read(ws->ssl, ext, 2);
            payload_lengths[payload_count] = ((size_t)ext[0] << 8) | (size_t)ext[1];
        }
        payloads[payload_count] = (char*)malloc(payload_lengths[payload_count]+1);
        int bytes_read = SSL_read(ws->ssl, payloads[payload_count], payload_lengths[payload_count]);
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

void* heartbeat_thread(void* arg){
    printf("Heartbeat thread started\n");
    struct websocket* ws = (struct websocket*)arg;
    const char* heartbeat_payload = "{\"op\":1,\"d\":null}";
    struct timespec ts;
    ts.tv_sec = ws->heartbeat_interval_ms / 1000;
    ts.tv_nsec = (ws->heartbeat_interval_ms % 1000) * 1000000;
    while(1){
        nanosleep(&ts, NULL);
        if(ws->heartbeat_interval_ms == 0){
            printf("Heartbeat thread exiting\n");
            break;
        }
        if(ws_send_text(ws, heartbeat_payload)){
            fprintf(stderr, "Failed to send heartbeat\n");
            break;
        }
        printf("Heartbeat sent\n");
    }
    return NULL;
}

int ws_start_heartbeat(struct websocket* ws, int interval_ms){
    ws->heartbeat_interval_ms = interval_ms;
    if(pthread_create(&ws->heartbeat_thread, NULL, heartbeat_thread, ws) != 0){
        fprintf(stderr, "Failed to create heartbeat thread\n");
        return 1;
    }
    return 0;
}

void ws_free_message(struct ws_message* msg){
    if(msg)
        free(msg->payload);
    free(msg);
}

void ws_close(struct websocket* ws){
    if(ws->ssl){
        unsigned char close_frame[6] = {0x88, 0x80};
        RAND_bytes(&close_frame[2], 4);
        pthread_mutex_lock(&ws->send_mutex);
        SSL_write(ws->ssl, close_frame, 6);
        ws->heartbeat_interval_ms = 0;
        pthread_mutex_unlock(&ws->send_mutex);
        pthread_join(ws->heartbeat_thread, NULL);
        SSL_shutdown(ws->ssl);
        SSL_free(ws->ssl);
        pthread_mutex_destroy(&ws->send_mutex);
    }
    if(ws->ctx)
        SSL_CTX_free(ws->ctx);
    if(ws->socket_fd != -1)
        close(ws->socket_fd);
}