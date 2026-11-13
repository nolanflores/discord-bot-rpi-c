#include "bot/plugin.h"

static char* helloworld_command(discord_rest* rest, const char* channel_id, const char* args){
    (void)args;
    return rest_send_text(rest, channel_id, "Hello, world!");
}

static const bot_command commands[] = {
    { -152191, helloworld_command }
};

static const bot_plugin plugin = {
    commands,
    sizeof(commands) / sizeof(commands[0]),
    "Hello World Plugin",
    "A simple plugin that responds with 'Hello, world!' when the command is invoked."
};

const bot_plugin* bot_plugin_init(void){
    return &plugin;
}
