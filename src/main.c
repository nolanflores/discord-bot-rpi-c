#include "discord/discord.h"
#include "bot/commands.h"
#include "core/SSL_context.h"
#include <locale.h>


int main(){
    if(ssl_context_create()) {
        fprintf(stderr, "Failed to create SSL context\n");
        return 1;
    }

    struct discord_bot bot;
    if(discord_init(&bot)){
        ssl_context_free();
        return 1;
    }

    setlocale(LC_NUMERIC, "");

    while(1){
        struct discord_event* event = discord_receive_event(&bot);
        if(!event){
            fprintf(stderr, "Failed to receive event\n");
            break;
        }
        if(event->type == MESSAGE_CREATE){
            if(event->content[0] == '!'){
                handle_command(&bot, event->channel_id, event->content);
            }
        }else if(event->type == CLOSE_CONNECTION){
            discord_free_event(event);
            fprintf(stderr, "Connection closed\n");
            break;
        }
        discord_free_event(event);
    }

    discord_cleanup(&bot);
    https_ctx_free();
    return 0;
}