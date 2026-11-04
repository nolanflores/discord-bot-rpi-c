#include "bot/commands.h"
#include "bot/helldivers.h"
#include "bot/help.h"
#include "core/plugin_loader.h"
#include <string.h>

enum command_hashes{
    CMD_SELL = 2090720021, CMD_BUDDY = 254689437, CMD_SOLD = 2090730903,
    CMD_BUST = 2090126755, CMD_SLAP = 2090727285, CMD_REACT = 273085876,
    CMD_PRAY = 2090626017, CMD_WHIP = 2090866941, CMD_TAP = 193506634,
    CMD_NUKE = 2090557720, CMD_NOTICE = 277905831,CMD_WINNER = 622798744,
    CMD_TOSS = 2090767086,

    CMD_VERSION = 1929407563, CMD_REPO = 2090684219, CMD_HELP = 2090324718,

    CMD_WAR = 193509903, CMD_MO = 5863617, CMD_CYBERSTAN = 353245488,
    CMD_DIVE = 2090185645//dive is not implemented yet
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
        //GIFS
        case CMD_SELL:
            response = rest_send_text(rest, channel_id, "https://tenor.com/view/say-what-gif-7766983");
            break;
        case CMD_BUDDY:
            response = rest_send_text(rest, channel_id, "https://tenor.com/view/hey-buddy-you-have-to-be-quiet-people-don%27t-like-you-hey-buddy-you-have-to-be-quiet-because-hey-buddy-you-have-to-be-quiet-because-people-don%27t-like-you-gif-3338348331317023398");
            break;
        case CMD_SOLD:
            response = rest_send_text(rest, channel_id, "https://cdn.discordapp.com/attachments/945070791949709362/1406421859397795840/IMG_4698.gif");
            break;
        case CMD_BUST:
            response = rest_send_text(rest, channel_id, "https://media.discordapp.net/attachments/945070791949709362/1146307918447255552/bust.gif");
            break;
        case CMD_SLAP:
            response = rest_send_text(rest, channel_id, "https://tenor.com/view/nic-cage-nicolas-cage-slap-mom-and-dad-inhale-gif-22714577");
            break;
        case CMD_REACT:
            response = rest_send_text(rest, channel_id, "https://tenor.com/view/soldier-soldier-tf2-tf2-soldier-gaming-gif-5369387296864231941");
            break;
        case CMD_PRAY:
            response = rest_send_text(rest, channel_id, "https://tenor.com/view/collusion-trump-jabba-the-hut-gif-13001238");
            break;
        case CMD_WHIP:
            response = rest_send_text(rest, channel_id, "https://tenor.com/view/whipping-johnny-rico-gif-19817777");
            break;
        case CMD_TAP:
            response = rest_send_text(rest, channel_id, "https://tenor.com/view/nemo-tap-glass-hello-gif-15153530");
            break;
        case CMD_NUKE:
            response = rest_send_text(rest, channel_id, "https://cdn.discordapp.com/attachments/945070791949709362/1389203749565890600/Yapyap.gif");
            break;
        case CMD_NOTICE:
            response = rest_send_text(rest, channel_id, "https://tenor.com/view/brendan-frasier-oscars-meme-glasses-gif-17794317351170463418");
            break;
        case CMD_WINNER:
            response = rest_send_text(rest, channel_id, "https://tenor.com/view/epic-win-gif-18390652");
            break;
        case CMD_TOSS:
            response = rest_send_text(rest, channel_id, "https://tenor.com/view/game-of-thrones-little-finger-petyr-baelish-baelish-drop-gif-4018898");
            break;
        //Plain text
        case CMD_VERSION:
            response = rest_send_text(rest, channel_id, "Discord Bot GNU C11 Edition");
            break;
        case CMD_REPO:
            response = rest_send_text(rest, channel_id, "https://github.com/nolanflores/discord-bot-rpi-c");
            break;
        case CMD_HELP:
            if(args == NULL)
                response = help_command(rest, channel_id);
            else if(strcmp(args, "hd2") == 0)
                response = help_helldivers(rest, channel_id);
            else if(strcmp(args, "info") == 0)
                response = help_info(rest, channel_id);
            break;
        //Commands with https requests
        case CMD_MO:
            response = helldivers_major_order(rest, channel_id);
            break;
        case CMD_WAR:
            response = helldivers_war_summary(rest, channel_id);
            break;
        case CMD_CYBERSTAN:
            //response = helldivers_cyberstan(rest, channel_id);
            response = rest_send_text(rest, channel_id, "It's Joe Over.");
            break;
        default:
            return 0;
    }
    if(response == NULL)
        return 1;
    free(response);
    return 0;
}