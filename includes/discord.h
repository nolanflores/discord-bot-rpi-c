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

int discord_bot_init(struct discord_bot* bot, const char* token);

void discord_bot_cleanup(struct discord_bot* bot);

#endif