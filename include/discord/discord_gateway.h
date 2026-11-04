#ifndef DISCORD_H
#define DISCORD_H

#include "core/https_socket.h"
#include "core/websocket.h"
#include "core/cJSON.h"
#include <pthread.h>

/*
 * Structure representing an active Discord gateway connection.
 * 
 * ws: The websocket connected to the Discord Gateway.
 * heartbeat_thread: The thread responsible for sending heartbeat messages to the Gateway.
 * mutex: Mutex for synchronizing access to shared bot state.
 * cond: Condition variable for signaling the heartbeat thread to stop.
 * heartbeat_interval: The interval in milliseconds between heartbeats, as specified by the Gateway.
 * event_s: The "s" field from the most recent Gateway event, used for heartbeats.
 */
typedef struct discord_gateway{
    websocket ws;
    pthread_t heartbeat_thread;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int heartbeat_interval;
    int event_s;
}discord_gateway;

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

typedef struct gateway_event{
    cJSON* json;
    char* id;
    char* token;
    char* channel_id;
    char* custom_id;
    char* content;
    enum event_type type;
}gateway_event;

int gateway_init(discord_gateway* bot);

gateway_event* gateway_receive_event(discord_gateway* bot);

void gateway_free_event(gateway_event* event);

void gateway_cleanup(discord_gateway* bot);

#endif