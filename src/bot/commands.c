#include "bot/commands.h"
#include "bot/help.h"
#include "bot/plugin_loader.h"
#include <string.h>

enum command_hashes{
    CMD_VERSION = 1929407563,
    CMD_REPO = 2090684219,
    CMD_HELP = 2090324718
};

static int djb2_hash(const char* command, const char** args){
    int hash = 5381;
    int c;
    while((c = *command++)){
        if(c == ' '){
            *args = command;
            break;
        }
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

int handle_command(discord_rest* rest, const char* channel_id, const char* content){
    const char* args = NULL;
    content++;//skip '!' prefix
    int hash = djb2_hash(content, &args);
    int plugin_result = plugin_loader_handle_command(rest, channel_id, hash, args);
    if(plugin_result >= 0)
        return plugin_result;

    char* response = NULL;
    switch(hash){
        case CMD_VERSION:
            response = rest_send_text(rest, channel_id, "Discord Bot GNU C11 Edition");
            break;
        case CMD_REPO:
            response = rest_send_text(rest, channel_id, "https://github.com/nolanflores/discord-bot-rpi-c");
            break;
        case CMD_HELP:
            response = plugin_loader_help(rest, channel_id);
            break;
        default:
            return 0;
    }
    if(response == NULL)
        return 1;
    free(response);
    return 0;
}