#ifndef DISCORD_REST_H
#define DISCORD_REST_H

#include "core/https_socket.h"

typedef struct discord_rest{
    https_socket socket;
}discord_rest;

int rest_connect(discord_rest* rest);

void rest_disconnect(discord_rest* rest);

char* rest_send_json(discord_rest* rest, const char* channel_id, const char* json);

char* rest_send_text(discord_rest* rest, const char* channel_id, const char* text);

char* rest_send_embed(discord_rest* rest, const char* channel_id, const char* title, const char* description, const int color);

#endif