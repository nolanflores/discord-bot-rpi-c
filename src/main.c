#include "discord.h"

int main(){
    https_ctx_init();

    struct discord_bot bot;
    if(discord_init(&bot)){
        https_ctx_free();
        return 1;
    }

    while(1){
        struct discord_event* event = discord_receive_event(&bot);
        if(!event){
            break;
        }
        if(event->type == MESSAGE_CREATE){
            if(strcmp(event->content, "!version") == 0){
                discord_send_message(&bot, event->channel_id, "Cumbot GNU C11 Edition");
            }
        }else if(event->type == CLOSE_CONNECTION){
            discord_free_event(event);
            break;
        }

        discord_free_event(event);
    }

    discord_cleanup(&bot);
    https_ctx_free();
    return 0;
}