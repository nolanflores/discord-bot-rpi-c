#ifndef DISCORD_H
#define DISCORD_H

#include "https_socket.h"
#include "websocket.h"
#include "cJSON.h"
#include <pthread.h>

struct discord_bot{
    struct addrinfo* rest_api;
    struct websocket ws;
    pthread_t heartbeat_thread;
    pthread_mutex_t send_mutex;
    int heartbeat_interval;
};

enum event_type{
    UNDEFINED_EVENT,
    MESSAGE_CREATE,
    INTERACTION_CREATE,
    CLOSE_CONNECTION
};

struct discord_event{
    cJSON* json;
    char* channel_id;
    char* custom_id;
    char* content;
    enum event_type type;
};

int discord_init(struct discord_bot* bot);

char* discord_send_message(struct discord_bot* bot, const char* channel, const char* content);

char* discord_send_embed(struct discord_bot* bot, const char* channel, const char* title, const char* description, const char* color_hex);

struct discord_event* discord_receive_event(struct discord_bot* bot);

void discord_free_event(struct discord_event* event);

void discord_cleanup(struct discord_bot* bot);

#endif