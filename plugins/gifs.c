#include "bot/plugin.h"

enum gif_hashes{
    CMD_SELL = 2090720021, CMD_BUDDY = 254689437, CMD_SOLD = 2090730903,
    CMD_BUST = 2090126755, CMD_SLAP = 2090727285, CMD_REACT = 273085876,
    CMD_PRAY = 2090626017, CMD_WHIP = 2090866941, CMD_TAP = 193506634,
    CMD_NUKE = 2090557720, CMD_NOTICE = 277905831,CMD_WINNER = 622798744,
    CMD_TOSS = 2090767086
};

static char* gif_sell(discord_rest* rest, const char* channel_id, const char* args){
    (void)args;
    return rest_send_text(rest, channel_id, "https://tenor.com/view/say-what-gif-7766983");
}
static char* gif_buddy(discord_rest* rest, const char* channel_id, const char* args){
    (void)args;
    return rest_send_text(rest, channel_id, "https://tenor.com/view/hey-buddy-you-have-to-be-quiet-people-don%27t-like-you-hey-buddy-you-have-to-be-quiet-because-hey-buddy-you-have-to-be-quiet-because-people-don%27t-like-you-gif-3338348331317023398");
}
static char* gif_sold(discord_rest* rest, const char* channel_id, const char* args){
    (void)args;
    return rest_send_text(rest, channel_id, "https://cdn.discordapp.com/attachments/945070791949709362/1406421859397795840/IMG_4698.gif");
}
static char* gif_bust(discord_rest* rest, const char* channel_id, const char* args){
    (void)args;
    return rest_send_text(rest, channel_id, "https://media.discordapp.net/attachments/945070791949709362/1146307918447255552/bust.gif");
}
static char* gif_slap(discord_rest* rest, const char* channel_id, const char* args){
    (void)args;
    return rest_send_text(rest, channel_id, "https://tenor.com/view/nic-cage-nicolas-cage-slap-mom-and-dad-inhale-gif-22714577");
}
static char* gif_react(discord_rest* rest, const char* channel_id, const char* args){
    (void)args;
    return rest_send_text(rest, channel_id, "https://tenor.com/view/soldier-soldier-tf2-tf2-soldier-gaming-gif-5369387296864231941");
}
static char* gif_pray(discord_rest* rest, const char* channel_id, const char* args){
    (void)args;
    return rest_send_text(rest, channel_id, "https://tenor.com/view/collusion-trump-jabba-the-hut-gif-13001238");
}
static char* gif_whip(discord_rest* rest, const char* channel_id, const char* args){
    (void)args;
    return rest_send_text(rest, channel_id, "https://tenor.com/view/whipping-johnny-rico-gif-19817777");
}
static char* gif_tap(discord_rest* rest, const char* channel_id, const char* args){
    (void)args;
    return rest_send_text(rest, channel_id, "https://tenor.com/view/nemo-tap-glass-hello-gif-15153530");
}
static char* gif_nuke(discord_rest* rest, const char* channel_id, const char* args){
    (void)args;
    return rest_send_text(rest, channel_id, "https://cdn.discordapp.com/attachments/945070791949709362/1389203749565890600/Yapyap.gif");
}
static char* gif_notice(discord_rest* rest, const char* channel_id, const char* args){
    (void)args;
    return rest_send_text(rest, channel_id, "https://tenor.com/view/brendan-frasier-oscars-meme-glasses-gif-17794317351170463418");
}
static char* gif_winner(discord_rest* rest, const char* channel_id, const char* args){
    (void)args;
    return rest_send_text(rest, channel_id, "https://tenor.com/view/epic-win-gif-18390652");
}
static char* gif_toss(discord_rest* rest, const char* channel_id, const char* args){
    (void)args;
    return rest_send_text(rest, channel_id, "https://tenor.com/view/game-of-thrones-little-finger-petyr-baelish-baelish-drop-gif-4018898");
}


static const bot_command commands[] = {
    { CMD_SELL, gif_sell },
    { CMD_BUDDY, gif_buddy },
    { CMD_SOLD, gif_sold },
    { CMD_BUST, gif_bust },
    { CMD_SLAP, gif_slap },
    { CMD_REACT, gif_react },
    { CMD_PRAY, gif_pray },
    { CMD_WHIP, gif_whip },
    { CMD_TAP, gif_tap },
    { CMD_NUKE, gif_nuke },
    { CMD_NOTICE, gif_notice },
    { CMD_WINNER, gif_winner },
    { CMD_TOSS, gif_toss }
};

static const bot_plugin plugin = {
    commands,
    sizeof(commands) / sizeof(commands[0])
};

const bot_plugin* bot_plugin_init(void){
    return &plugin;
}