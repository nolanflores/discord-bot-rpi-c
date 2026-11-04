#include "discord/discord_gateway.h"
#include "discord/discord_rest.h"
#include "discord/token.h"
#include "bot/commands.h"
#include "core/plugin_loader.h"
#include <locale.h>

#ifndef PLUGIN_DIRECTORY
#define PLUGIN_DIRECTORY "./plugins"
#endif

int main(){
    load_token();
    setlocale(LC_NUMERIC, "");
    https_ctx_init();

    plugin_loader_load(PLUGIN_DIRECTORY);

    discord_rest rest;
    if(rest_connect(&rest)){
        plugin_loader_unload();
        https_ctx_free();
        return 1;
    }
    discord_gateway gw;
    if(gateway_init(&gw)){
        rest_disconnect(&rest);
        plugin_loader_unload();
        https_ctx_free();
        return 1;
    }

    while(1){
        gateway_event* event = gateway_receive_event(&gw);
        if(!event){
            fprintf(stderr, "Failed to receive event\n");
            break;
        }
        if(event->type == MESSAGE_CREATE){
            if(event->content[0] == '!'){
                handle_command(&rest, event->channel_id, event->content);
            }
        }else if(event->type == CLOSE_CONNECTION){
            gateway_free_event(event);
            fprintf(stderr, "Connection closed\n");
            break;
        }
        gateway_free_event(event);
    }

    gateway_cleanup(&gw);
    rest_disconnect(&rest);
    plugin_loader_unload();
    https_ctx_free();
    return 0;
}