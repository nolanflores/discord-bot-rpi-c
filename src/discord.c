#include "discord.h"
#include "cJSON.h"
#include "discord_token.h"
#include <time.h>

static void* heartbeat_loop(void* arg){
    struct discord_bot* bot = (struct discord_bot*)arg;

    const char* heartbeat_payload = "{\"op\":1,\"d\":null}";

    struct timespec ts;
    ts.tv_sec = bot->heartbeat_interval / 1000;
    ts.tv_nsec = (bot->heartbeat_interval % 1000) * 1000000;

    while(1){
        nanosleep(&ts, NULL);
        if(bot->heartbeat_interval == 0)
            break;
        pthread_mutex_lock(&bot->send_mutex);
        ws_send_text(&bot->ws, heartbeat_payload);
        pthread_mutex_unlock(&bot->send_mutex);
    }

    return NULL;
}

int discord_bot_init(struct discord_bot* bot, const char* token){
    bot->rest_api = https_dns_lookup("discord.com", "443");
    if(!bot->rest_api)
        return 1;
    if(ws_connect(&bot->ws, "gateway.discord.gg", "443")){
        freeaddrinfo(bot->rest_api);
        return 1;
    }
    
    struct ws_message* msg = ws_receive(&bot->ws);
    if(!msg){
        freeaddrinfo(bot->rest_api);
        ws_close(&bot->ws);
        return 1;
    }
    cJSON* json = cJSON_Parse(msg->payload);
    cJSON* d = cJSON_GetObjectItem(json, "d");
    bot->heartbeat_interval = cJSON_GetObjectItem(d, "heartbeat_interval")->valueint;
    cJSON_Delete(json);
    ws_free_message(msg);
    
    if(pthread_mutex_init(&bot->send_mutex, NULL) || pthread_create(&bot->heartbeat_thread, NULL, heartbeat_loop, bot)){
        freeaddrinfo(bot->rest_api);
        ws_close(&bot->ws);
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
                    "\"browser\":\"cumbot\","
                    "\"device\":\"cumbot\""
                "}"
            "}"
        "}",
        DISCORD_TOKEN,
        (1 << 9) | (1 << 12) | (1 << 15)//GUILD_MESSAGES, DIRECT_MESSAGES, MESSAGE_CONTENT
    );
    pthread_mutex_lock(&bot->send_mutex);
    ws_send_text(&bot->ws, identify_payload);
    pthread_mutex_unlock(&bot->send_mutex);
}

void discord_bot_cleanup(struct discord_bot* bot){
    bot->heartbeat_interval = 0;
    pthread_join(bot->heartbeat_thread, NULL);
    pthread_mutex_destroy(&bot->send_mutex);
    ws_close(&bot->ws);
    freeaddrinfo(bot->rest_api);
}