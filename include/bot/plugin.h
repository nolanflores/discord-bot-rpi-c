#ifndef BOT_PLUGIN_H
#define BOT_PLUGIN_H

#include "discord/discord_rest.h"

typedef char* (*bot_command_handler)(
    discord_rest* rest,
    const char* channel_id,
    const char* args
);

typedef struct bot_command{
    int hash;
    bot_command_handler handler;
}bot_command;

typedef struct bot_plugin{
    const bot_command* commands;
    size_t command_count;
}bot_plugin;

typedef const bot_plugin* (*bot_plugin_init_function)(void);

static inline int bot_command_hash(const char* command){
    int hash = 5381;
    int c;
    while((c = *command++))
        hash = ((hash << 5) + hash) + c;
    return hash;
}

#endif
