#include <stdio.h>
#include "https_socket.h"
#include "websocket.h"
#include "discord_token.h"
#include "cJSON.h"
#include <pthread.h>
#include <time.h>

pthread_mutex_t send_mutex = PTHREAD_MUTEX_INITIALIZER;
int heartbeat_interval = 0;

void* heartbeat_loop(void* arg){
    struct websocket* ws = (struct websocket*)arg;

    const char* heartbeat_payload = "{\"op\":1,\"d\":null}";

    struct timespec ts;
    ts.tv_sec = heartbeat_interval / 1000;
    ts.tv_nsec = (heartbeat_interval % 1000) * 1000000;

    while(1){
        nanosleep(&ts, NULL);
        if(heartbeat_interval == 0)
            break;
        pthread_mutex_lock(&send_mutex);
        ws_send_text(ws, heartbeat_payload);
        pthread_mutex_unlock(&send_mutex);
        printf("Sent heartbeat\n");
    }

    return NULL;
}

void kys(struct https_socket* hs){
    const char* channel = "985252976622985249";
    const char* content = "{\"content\":\"Go fuck yourself.\"}";
    char request[2048];
    snprintf(request, 2048,
        "POST /api/v10/channels/%s/messages HTTP/1.1\r\n"
        "Host: discord.com\r\n"
        "Authorization: %s\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "Connection: keep-alive\r\n\r\n"
        "%s",
        channel, DISCORD_TOKEN, (int)strlen(content), content
    );
    char* response = https_send(hs, request);
    if(response){
        printf("%s\n", response);
        free(response);
    }else{
        printf("Failed to get response\n");
    }
}

int main(){
    https_ctx_init();

    struct https_socket hs;
    if(https_connect(&hs, "discord.com", "443")){
        https_ctx_free();
        return 1;
    }

    struct websocket ws;
    if(ws_connect(&ws, "gateway.discord.gg", "443")){
        https_close(&hs);
        https_ctx_free();
        return 1;
    }

    struct ws_message* msg = ws_receive(&ws);
    if(!msg){
        printf("Failed to receive message\n");
        https_close(&hs);
        https_ctx_free();
        return 1;
    }
    cJSON* json = cJSON_ParseWithLength(msg->payload, msg->payload_len);
    cJSON* d = cJSON_GetObjectItemCaseSensitive(json, "d");
    cJSON* interval = cJSON_GetObjectItemCaseSensitive(d, "heartbeat_interval");
    heartbeat_interval = interval->valueint;
    cJSON_Delete(json);
    ws_free_message(msg);

    pthread_t heartbeat_thread;
    if(pthread_create(&heartbeat_thread, NULL, heartbeat_loop, &ws)){
        printf("Failed to create heartbeat thread\n");
        https_close(&hs);
        https_ctx_free();
        return 1;
    }

    char identify_payload[512];
    snprintf(identify_payload, 512,
        "{\"op\":2,\"d\":{\"token\":\"%s\",\"intents\":%d,\"properties\":{\"os\":\"linux\",\"browser\":\"cumbot\",\"device\":\"cumbot\"}}}",
        DISCORD_TOKEN, (1 << 9) | (1 << 12) | (1 << 15)
    );
    pthread_mutex_lock(&send_mutex);
    ws_send_text(&ws, identify_payload);
    pthread_mutex_unlock(&send_mutex);

    while(1){
        struct ws_message* message = ws_receive(&ws);
        if(!message){
            printf("Failed to receive message\n");
            break;
        }
        cJSON* mjson = cJSON_ParseWithLength(message->payload, message->payload_len);
        cJSON* t = cJSON_GetObjectItemCaseSensitive(mjson, "t");
        if(t && t->valuestring && strcmp(t->valuestring, "MESSAGE_CREATE") == 0){
            cJSON* d = cJSON_GetObjectItemCaseSensitive(mjson, "d");
            cJSON* content = cJSON_GetObjectItemCaseSensitive(d, "content");
            char* msg_content = content->valuestring;
            if(msg_content && strcmp(msg_content, "Kill yourself") == 0){
                kys(&hs);
            }else{
                printf("Received message: %s\n", msg_content);
            }
        }
        cJSON_Delete(mjson);
        ws_free_message(message);
    }

    https_close(&hs);
    https_ctx_free();
    return 0;
}