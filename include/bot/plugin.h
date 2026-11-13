#ifndef BOT_PLUGIN_H
#define BOT_PLUGIN_H

#include "discord/discord_rest.h"

typedef char* (*bot_command_handler)(discord_rest* rest, const char* channel_id, const char* args);

typedef struct bot_command{
    int hash;
    bot_command_handler handler;
}bot_command;

typedef struct bot_plugin{
    const bot_command* commands;
    size_t command_count;
    const char* name;
    const char* help;
}bot_plugin;

typedef const bot_plugin* (*bot_plugin_init_function)(void);

#endif
