#ifndef DISCORD_H
#define DISCORD_H

#include "core/https_socket.h"
#include "core/websocket.h"
#include "core/cJSON.h"
#include <pthread.h>

/*
 * Structure representing an active Discord bot connection.
 * 
 * rest_api: The addrinfo list for the Discord REST API endpoint.
 * ws: The websocket connected to the Discord Gateway.
 * heartbeat_thread: The thread responsible for sending heartbeat messages to the Gateway.
 * mutex: Mutex for synchronizing access to shared bot state.
 * cond: Condition variable for signaling the heartbeat thread to stop.
 * heartbeat_interval: The interval in milliseconds between heartbeats, as specified by the Gateway.
 * event_s: The "s" field from the most recent Gateway event, used for heartbeats.
 */
struct discord_bot{
    struct addrinfo* rest_api;
    struct websocket ws;
    pthread_t heartbeat_thread;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int heartbeat_interval;
    int event_s;
};

/*
 * Enum representing the type of received Discord events.
 * There are more types than this, but these are relevant ones for this bot.
 * Events that are not relevant are categorized as UNDEFINED_EVENT and can be ignored.
 */
enum event_type{
    UNDEFINED_EVENT,
    MESSAGE_CREATE,
    INTERACTION_CREATE,
    CLOSE_CONNECTION
};

struct discord_event{
    cJSON* json;
    char* id;
    char* token;
    char* channel_id;
    char* custom_id;
    char* content;
    enum event_type type;
};

int discord_init(struct discord_bot* bot);

char* discord_send_message(struct discord_bot* bot, const char* channel, const char* content);

char* discord_send_embed(struct discord_bot* bot, const char* channel, const char* title, const char* description, const int color_hex);

struct discord_event* discord_receive_event(struct discord_bot* bot);

void discord_free_event(struct discord_event* event);

void discord_cleanup(struct discord_bot* bot);

#endif