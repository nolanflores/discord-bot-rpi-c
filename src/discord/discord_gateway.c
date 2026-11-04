#include "discord/discord_gateway.h"
#include "discord/token.h"
#include <time.h>
#include <netdb.h>

/*
 * This function runs in a loop, sleeping for the specified heartbeat interval,
 * and sending a heartbeat message to the Gateway each time.
 * The loop can be exited by setting the heartbeat_interval to 0 and signaling the condition variable.
 */
static void* heartbeat_loop(void* arg){
    discord_gateway* gw = (discord_gateway*)arg;

    char heartbeat_payload[64];

    struct timespec ts;
    int interval_sec = gw->heartbeat_interval / 1000;
    int interval_nsec = (gw->heartbeat_interval % 1000) * 1000000;

    while(1){
        clock_gettime(0, &ts);
        ts.tv_sec += interval_sec;
        ts.tv_nsec += interval_nsec;
        if(ts.tv_nsec >= 1000000000){
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000000000;
        }

        pthread_mutex_lock(&gw->mutex);
        pthread_cond_timedwait(&gw->cond, &gw->mutex, &ts);

        if(gw->heartbeat_interval == 0){
            pthread_mutex_unlock(&gw->mutex);
            break;
        }
        if(gw->event_s == -1){
            snprintf(heartbeat_payload, 64,"{\"op\":1,\"d\":null}");
        }else{
            snprintf(heartbeat_payload, 64,"{\"op\":1,\"d\":%d}", gw->event_s);
        }
        ws_send_text(&gw->ws, heartbeat_payload);
        pthread_mutex_unlock(&gw->mutex);
    }

    return NULL;
}


/* ============================================================================
 * Public Functions
 * ============================================================================
*/


int gateway_init(discord_gateway* gw){
    if(ws_connect(&gw->ws, "gateway.discord.gg", "443")){
        ws_close(&gw->ws);
        return 1;
    }
    
    struct ws_message* msg = ws_receive(&gw->ws);
    if(!msg){
        ws_close(&gw->ws);
        return 1;
    }
    gw->event_s = -1;
    cJSON* json = cJSON_Parse(msg->payload);
    cJSON* d = cJSON_GetObjectItem(json, "d");
    gw->heartbeat_interval = cJSON_GetObjectItem(d, "heartbeat_interval")->valueint;
    cJSON_Delete(json);
    ws_free_message(msg);
    
    if(pthread_mutex_init(&gw->mutex, NULL) || 
        pthread_cond_init(&gw->cond, NULL) ||
        pthread_create(&gw->heartbeat_thread, NULL, heartbeat_loop, gw)
    ){
        ws_close(&gw->ws);
        return 1;
    }

    char identify_payload[512];
    snprintf(identify_payload, 512,
        "{"
            "\"op\":2,"
            "\"d\":{"
                "\"token\":\"%s\","
                "\"intents\":%d,"
                "\"properties\":{"
                    "\"os\":\"linux\","
                    "\"browser\":\"rpi_bot\","
                    "\"device\":\"rpi_bot\""
                "}"
            "}"
        "}",
        get_token(),
        (1 << 9) | (1 << 12) | (1 << 15)//GUILD_MESSAGES, DIRECT_MESSAGES, MESSAGE_CONTENT
    );
    pthread_mutex_lock(&gw->mutex);
    ws_send_text(&gw->ws, identify_payload);
    pthread_mutex_unlock(&gw->mutex);
    return 0;
}



gateway_event* gateway_receive_event(discord_gateway* gw){
    struct ws_message* message = ws_receive(&gw->ws);
    if(!message)
        return NULL;
    if((message->opcode != 1 && message->opcode != 8) || (message->payload && strncmp(message->payload, "{\"op\":11", 8) == 0)){
        ws_free_message(message);
        return NULL;
    }
    gateway_event* event = (gateway_event*)calloc(1, sizeof(gateway_event));
    if(message->opcode == 8){
        ws_free_message(message);
        event->type = CLOSE_CONNECTION;
        return event;
    }
    cJSON* json = cJSON_Parse(message->payload);
    ws_free_message(message);
    if(!json){
        fputs("Failed to parse event JSON\n", stderr);
        free(event);
        return NULL;
    }
    event->json = json;
    cJSON* t = cJSON_GetObjectItemCaseSensitive(json, "t");
    if(!t || !t->valuestring){
        return event;
    }
    cJSON* s = cJSON_GetObjectItemCaseSensitive(json, "s");
    pthread_mutex_lock(&gw->mutex);
    gw->event_s = s->valueint;
    pthread_mutex_unlock(&gw->mutex);
    if(strcmp(t->valuestring, "MESSAGE_CREATE") == 0){
        event->type = MESSAGE_CREATE;
        cJSON* d = cJSON_GetObjectItemCaseSensitive(json, "d");
        cJSON* content = cJSON_GetObjectItemCaseSensitive(d, "content");
        cJSON* channel_id = cJSON_GetObjectItemCaseSensitive(d, "channel_id");
        if(!content || !channel_id){
            fputs("Malformed MESSAGE_CREATE event\n", stderr);
            cJSON_Delete(json);
            free(event);
            return NULL;
        }
        event->content = content->valuestring;
        event->channel_id = channel_id->valuestring;
        return event;
    }else if(strcmp(t->valuestring, "INTERACTION_CREATE") == 0){
        event->type = INTERACTION_CREATE;
        cJSON* d = cJSON_GetObjectItemCaseSensitive(json, "d");
        cJSON* id = cJSON_GetObjectItemCaseSensitive(d, "id");
        event->id = id->valuestring;
        cJSON* token = cJSON_GetObjectItemCaseSensitive(d, "token");
        event->token = token->valuestring;
        cJSON* data = cJSON_GetObjectItemCaseSensitive(d, "data");
        cJSON* custom_id = cJSON_GetObjectItemCaseSensitive(data, "custom_id");
        event->custom_id = custom_id->valuestring;
        cJSON* message = cJSON_GetObjectItemCaseSensitive(d, "message");
        cJSON* content = cJSON_GetObjectItemCaseSensitive(message, "content");
        cJSON* channel_id = cJSON_GetObjectItemCaseSensitive(message, "channel_id");
        event->content = content->valuestring;
        event->channel_id = channel_id->valuestring;
    }
    return event;
}



void gateway_free_event(gateway_event* event){
    if(event){
        if(event->json)
            cJSON_Delete(event->json);
        free(event);
    }
}

void gateway_cleanup(discord_gateway* gw){
    pthread_mutex_lock(&gw->mutex);
    gw->heartbeat_interval = 0;
    pthread_cond_signal(&gw->cond);
    pthread_mutex_unlock(&gw->mutex);
    pthread_join(gw->heartbeat_thread, NULL);
    pthread_mutex_destroy(&gw->mutex);
    ws_close(&gw->ws);
}