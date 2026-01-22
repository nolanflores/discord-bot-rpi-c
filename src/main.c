#include "discord.h"
#include "commands.h"
#include <signal.h>

volatile sig_atomic_t stop = 0;

void handle_sigint(int sig) {
    (void)sig;
    stop = 1;
}

int main(){
    signal(SIGINT, handle_sigint);

    https_ctx_init();

    struct discord_bot bot;
    if(discord_init(&bot)){
        https_ctx_free();
        return 1;
    }

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