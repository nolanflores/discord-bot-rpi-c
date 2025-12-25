#include <stdio.h>
#include <openssl/ssl.h>
#include <openssl/rand.h>
#include "websocket.h"
#include "cJSON.h"
#include "discord_token.h"

int main(){
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    struct websocket ws;

    if(ws_tcp_connect(&ws)){
        return 1;
    }
    if(ws_tls_connect(&ws)){
        return 1;
    }
    if(ws_handshake(&ws)){
        return 1;
    }

    char message[256];
    snprintf(message, 256, "{\"op\":2,\"d\":{\"token\":\"%s\",\"properties\":{\"os\":\"linux\",\"browser\":\"my_c_client\",\"device\":\"my_c_client\"},\"intents\":33280}}", DISCORD_TOKEN);    
    if(ws_send_text(&ws, message)){
        ws_close(&ws);
        return 1;
    }
    
    while(1){
        struct ws_message* response = ws_receive(&ws);
        if(!response){
            fprintf(stderr, "Failed to receive message\n");
            break;
        }
        if(response->opcode == 8){
            ws_free_message(response);
            break;
        }
        cJSON* json = cJSON_Parse(response->payload);
        if(!json){
            fprintf(stderr, "Failed to parse JSON\n");
            ws_free_message(response);
            continue;
        }
        int op = cJSON_GetObjectItemCaseSensitive(json, "op")->valueint;
        if(op == 0){
            if(strcmp(cJSON_GetObjectItemCaseSensitive(json, "t")->valuestring, "MESSAGE_CREATE") == 0){
                cJSON* d = cJSON_GetObjectItemCaseSensitive(json, "d");
                char* txt = cJSON_GetObjectItemCaseSensitive(d, "content")->valuestring;
                printf("Received data: %s\n", txt);
                if(strcmp(txt, "close") == 0){
                    ws_free_message(response);
                    cJSON_Delete(json);
                    break;
                }
            }
        }else if(op == 9){
            fprintf(stderr, "Invalid session\n");
            ws_close(&ws);
            ws_free_message(response);
            cJSON_Delete(json);
            break;
        }else if(op == 10){
            int heartbeat_interval = cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(json, "d"), "heartbeat_interval")->valueint;
            if(ws_start_heartbeat(&ws, heartbeat_interval)){
                ws_free_message(response);
                cJSON_Delete(json);
                break;
            }
        }
        cJSON_Delete(json);
        ws_free_message(response);
    }

    //ws_free_message(response);

    ws_close(&ws);

    return 0;
}