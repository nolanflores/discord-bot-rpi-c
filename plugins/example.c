#include "bot/plugin.h"

enum command_hashes{
    CMD_HELLOWORLD = -152191,
    CMD_ECHO = 2090214596
};

static char* helloworld_command(discord_rest* rest, const char* channel_id, const char* args){
    (void)args;
    return rest_send_text(rest, channel_id, "Hello, world!");
}

static char* echo_command(discord_rest* rest, const char* channel_id, const char* args){
    return rest_send_text(rest, channel_id, args);
}

static const bot_command commands[] = {
    { CMD_HELLOWORLD, helloworld_command },
    { CMD_ECHO, echo_command }
};

static const bot_plugin plugin = {
    commands,
    sizeof(commands) / sizeof(commands[0]),
    "Basic Commands",
    "`!helloworld` `!echo <message>`"
};

const bot_plugin* bot_plugin_init(void){
    return &plugin;
}
