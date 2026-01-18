#ifndef DISCORD_H
#define DISCORD_H

#include "https_socket.h"
#include "websocket.h"
#include <pthread.h>

struct discord_bot{
    struct addrinfo* rest_api;
    struct websocket ws;
    pthread_t heartbeat_thread;
    pthread_mutex_t send_mutex;
    int heartbeat_interval;
};

int discord_init(struct discord_bot* bot);

char* discord_send_message(struct discord_bot* bot, const char* channel, const char* content);

void discord_cleanup(struct discord_bot* bot);

#endif