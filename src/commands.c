#include "commands.h"

enum command_hashes{
    CMD_SELL = 2090720021,
    CMD_BUDDY = 254689437,
    CMD_SOLD = 2090730903,
    CMD_BUST = 2090126755,
    CMD_SLAP = 2090727285,
    CMD_REACT = 273085876,
    CMD_PRAY = 2090626017,
    CMD_WHIP = 2090866941,
    CMD_TAP = 193506634,
    CMD_NUKE = 2090557720,
    CMD_NOTICE = 277905831,
    CMD_WINNER = 622798744,

    CMD_VERSION = 1929407563,
    CMD_REPO = 2090684219,
    CMD_HELP = 2090324718,

    CMD_SLOTS = 274539162,
    CMD_WAR = 193509903,
    CMD_MO = 5863617,

    CMD_PORN = 2090623300,
    CMD_RULE34 = 441269444
};

static int djb2_hash(const char* command){
    int hash = 5381;
    int c;
    while((c = *command++)){
        if(c == ' ')
            break;
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

static char* war_command(struct discord_bot* bot, const char* channel_id){
    struct https_socket sock;
    if(https_connect(&sock, "helldiverstrainingmanual.com", "443")){
        return NULL;
    }
    char request[512];
    snprintf(request, 512,
        "GET /api/v1/war/campaign HTTP/1.1\r\n"
        "Host: %s\r\n"
        "User-Agent: Cumbot/1.0\r\n"
        "\r\n",
        sock.hostname
    );
    char* response = https_send(&sock, request);
    if(!response){
        https_close(&sock);
        return NULL;
    }
    struct cJSON* json = cJSON_Parse(response);
    if(!json){
        free(response);
        https_close(&sock);
        return NULL;
    }
    int array_size = cJSON_GetArraySize(json);
    struct cJSON* topFive[5];
    int divers[5] = {0};
    for(int i = 0; i < array_size; i++){
        struct cJSON* item = cJSON_GetArrayItem(json, i);
        struct cJSON* players = cJSON_GetObjectItemCaseSensitive(item, "players");
        for(int j = 0; j < 5; j++){
            if(players->valueint > divers[j]){
                for(int k = 4; k > j; k--){
                    divers[k] = divers[k-1];
                    topFive[k] = topFive[k-1];
                }
                divers[j] = players->valueint;
                topFive[j] = item;
                break;
            }
        }
    }
    char message[1024];
    int message_len = 0;
    for(int i = 0; i < 5; i++){
        struct cJSON* name = cJSON_GetObjectItemCaseSensitive(topFive[i], "name");
        struct cJSON* faction = cJSON_GetObjectItemCaseSensitive(topFive[i], "faction");
        struct cJSON* percentage = cJSON_GetObjectItemCaseSensitive(topFive[i], "percentage");
        message_len += snprintf(message + message_len, 1024 - message_len, "%s | %s\\n%d Current Divers\\n%.2f%% Liberated%s", name->valuestring, faction->valuestring, divers[i], percentage->valuedouble, i < 4 ? "\\n\\n" : "");
    }
    cJSON_Delete(json);
    free(response);
    https_close(&sock);
    return discord_send_embed(bot, channel_id, "Active Planets", message, 0xFFE900);
}

static char* major_order_command(struct discord_bot* bot, const char* channel_id){
    struct https_socket sock;
    if(https_connect(&sock, "helldiverstrainingmanual.com", "443")){
        return NULL;
    }
    char request[512];
    snprintf(request, 512,
        "GET /api/v1/war/major-orders HTTP/1.1\r\n"
        "Host: %s\r\n"
        "User-Agent: Cumbot/1.0\r\n"
        "\r\n",
        sock.hostname
    );
    char* response = https_send(&sock, request);
    if(!response){
        https_close(&sock);
        return NULL;
    }
    struct cJSON* json = cJSON_Parse(response);
    if(!json){
        free(response);
        https_close(&sock);
        return NULL;
    }
    char message[1024];
    int message_len = 0;
    int array_size = cJSON_GetArraySize(json);
    for(int i = 0; i < array_size; i++){
        struct cJSON* item = cJSON_GetArrayItem(json, i);
        struct cJSON* setting = cJSON_GetObjectItemCaseSensitive(item, "setting");
        struct cJSON* overrideTitle = cJSON_GetObjectItemCaseSensitive(setting, "overrideTitle");
        struct cJSON* overrideBrief = cJSON_GetObjectItemCaseSensitive(setting, "overrideBrief");
        struct cJSON* expiresIn = cJSON_GetObjectItemCaseSensitive(item, "expiresIn");
        int days = expiresIn->valueint / 86400;
        int hours = (expiresIn->valueint % 86400) / 3600;
        message_len += snprintf(message + message_len, 1024 - message_len, "%s\\n%s\\nExpires in %d days and %d hours.\\n", overrideTitle->valuestring, overrideBrief->valuestring, days, hours);
    }
    cJSON_Delete(json);
    free(response);
    https_close(&sock);
    return discord_send_embed(bot, channel_id, "High Command Orders", message, 0xFFE900);
}

void handle_command(struct discord_bot* bot, const char* channel_id, const char* content){
    content++;//skip '!' prefix
    int hash = djb2_hash(content);

    char* res = NULL;
    switch(hash){
        //GIFS
        case CMD_SELL:
            res = discord_send_message(bot, channel_id, "https://tenor.com/view/say-what-gif-7766983");
            break;
        case CMD_BUDDY:
            res = discord_send_message(bot, channel_id, "https://tenor.com/view/hey-buddy-you-have-to-be-quiet-people-don%27t-like-you-hey-buddy-you-have-to-be-quiet-because-hey-buddy-you-have-to-be-quiet-because-people-don%27t-like-you-gif-3338348331317023398");
            break;
        case CMD_SOLD:
            res = discord_send_message(bot, channel_id, "https://cdn.discordapp.com/attachments/945070791949709362/1406421859397795840/IMG_4698.gif");
            break;
        case CMD_BUST:
            res = discord_send_message(bot, channel_id, "https://media.discordapp.net/attachments/945070791949709362/1146307918447255552/bust.gif");
            break;
        case CMD_SLAP:
            res = discord_send_message(bot, channel_id, "https://tenor.com/view/nic-cage-nicolas-cage-slap-mom-and-dad-inhale-gif-22714577");
            break;
        case CMD_REACT:
            res = discord_send_message(bot, channel_id, "https://tenor.com/view/soldier-soldier-tf2-tf2-soldier-gaming-gif-5369387296864231941");
            break;
        case CMD_PRAY:
            res = discord_send_message(bot, channel_id, "https://tenor.com/view/collusion-trump-jabba-the-hut-gif-13001238");
            break;
        case CMD_WHIP:
            res = discord_send_message(bot, channel_id, "https://tenor.com/view/whipping-johnny-rico-gif-19817777");
            break;
        case CMD_TAP:
            res = discord_send_message(bot, channel_id, "https://tenor.com/view/nemo-tap-glass-hello-gif-15153530");
            break;
        case CMD_NUKE:
            res = discord_send_message(bot, channel_id, "https://cdn.discordapp.com/attachments/945070791949709362/1389203749565890600/Yapyap.gif");
            break;
        case CMD_NOTICE:
            res = discord_send_message(bot, channel_id, "https://tenor.com/view/brendan-frasier-oscars-meme-glasses-gif-17794317351170463418");
            break;
        case CMD_WINNER:
            res = discord_send_message(bot, channel_id, "https://tenor.com/view/epic-win-gif-18390652");
            break;
        //Plain text
        case CMD_VERSION:
            res = discord_send_message(bot, channel_id, "Cumbot GNU C11 Edition");
            break;
        case CMD_REPO:
            res = discord_send_message(bot, channel_id, "https://github.com/nolanflores/discord-bot-rpi-c");
            break;
        case CMD_HELP:
            res = discord_send_embed(bot, channel_id, "Available Commands", "!sell\\n!buddy\\n!sold\\n!bust\\n!slap\\n!react\\n!pray\\n!whip\\n!tap\\n!nuke\\n!notice\\n!winner\\n!mo\\n!war\\n!version\\n!repo", 0x192930);
            break;
        //Commands with https requests
        case CMD_MO:
            res = major_order_command(bot, channel_id);
            if(!res)
                res = discord_send_message(bot, channel_id, "Failed to fetch major orders.");
            break;
        case CMD_WAR:
            res = war_command(bot, channel_id);
            if(!res)
                res = discord_send_message(bot, channel_id, "Failed to fetch war data.");
            break;
        default:
            break;
    }
    free(res);
}