#include "discord.h"
#include "discord_token.h"
#include <time.h>
#include <netdb.h>

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

int discord_init(struct discord_bot* bot){
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
    return 0;
}

struct discord_event* discord_receive_event(struct discord_bot* bot){
    struct ws_message* message = ws_receive(&bot->ws);
    if(!message)
        return NULL;
    if(message->opcode != 1 && message->opcode != 8){
        ws_free_message(message);
        return NULL;
    }
    struct discord_event* event = (struct discord_event*)calloc(1, sizeof(struct discord_event));
    if(message->opcode == 8){
        ws_free_message(message);
        event->type = CLOSE_CONNECTION;
        return event;
    }
    cJSON* json = cJSON_Parse(message->payload);
    ws_free_message(message);
    if(!json){
        fprintf(stderr, "Failed to parse event JSON\n");
        free(event);
        return NULL;
    }
    event->json = json;
    cJSON* t = cJSON_GetObjectItemCaseSensitive(json, "t");
    if(!t || !t->valuestring){
        return event;
    }
    if(strcmp(t->valuestring, "MESSAGE_CREATE") == 0){
        event->type = MESSAGE_CREATE;
        cJSON* d = cJSON_GetObjectItemCaseSensitive(json, "d");
        cJSON* content = cJSON_GetObjectItemCaseSensitive(d, "content");
        cJSON* channel_id = cJSON_GetObjectItemCaseSensitive(d, "channel_id");
        if(!content || !channel_id){
            fprintf(stderr, "Malformed MESSAGE_CREATE event\n");
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

void discord_free_event(struct discord_event* event){
    if(event){
        if(event->json)
            cJSON_Delete(event->json);
        free(event);
    }
}

static char* discord_send(struct discord_bot* bot, const char* request){
    struct https_socket hs;
    if(https_connect_addrinfo(&hs, bot->rest_api)){
        return NULL;
    }
    char* response = https_send(&hs, request);
    https_close(&hs);
    return response;
}

char* discord_send_message(struct discord_bot* bot, const char* channel_id, const char* content){
    char request[8192];
    snprintf(request, 8192,
        "POST /api/v10/channels/%s/messages HTTP/1.1\r\n"
        "Host: discord.com\r\n"
        "Authorization: %s\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %zu\r\n"
        "\r\n"
        "{\"content\":\"%s\"}",
        channel_id,
        DISCORD_TOKEN,
        strlen(content) + 14,
        content
    );
    return discord_send(bot, request);
}

char* discord_send_embed(struct discord_bot* bot, const char* channel_id, const char* title, const char* description, const int color_hex){
    char content[6144];
    size_t content_length = snprintf(content, 6144,
        "{\"embeds\":[{\"title\":\"%s\",\"description\":\"%s\",\"color\":%d}]}",
        title,
        description,
        color_hex
    );
    char request[8192];
    snprintf(request, 8192,
        "POST /api/v10/channels/%s/messages HTTP/1.1\r\n"
        "Host: discord.com\r\n"
        "Authorization: %s\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %zu\r\n"
        "\r\n%s",
        channel_id,
        DISCORD_TOKEN,
        content_length,
        content
    );
    return discord_send(bot, request);
}

void discord_cleanup(struct discord_bot* bot){
    bot->heartbeat_interval = 0;
    pthread_join(bot->heartbeat_thread, NULL);
    pthread_mutex_destroy(&bot->send_mutex);
    ws_close(&bot->ws);
    freeaddrinfo(bot->rest_api);
}