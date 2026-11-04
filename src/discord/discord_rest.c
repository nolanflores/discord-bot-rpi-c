#include "discord/discord_rest.h"
#include "discord/token.h"

int rest_connect(discord_rest* rest){
    return https_connect(&rest->socket, "discord.com", "443");
}
    
void rest_disconnect(discord_rest* rest){
    https_close(&rest->socket);
}

char* rest_send_json(discord_rest* rest, const char* channel_id, const char* raw_json){
    char request[8192];
    snprintf(request, 8192,
        "POST /api/v10/channels/%s/messages HTTP/1.1\r\n"
        "Host: discord.com\r\n"
        "Authorization: %s\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %zu\r\n"
        "\r\n%s",
        channel_id,
        get_token(),
        strlen(raw_json),
        raw_json
    );
    return https_send(&rest->socket, request);
}

char* rest_send_text(discord_rest* rest, const char* channel_id, const char* content){
    char request[6144];
    snprintf(request, 6144,"{\"content\":\"%s\"}", content);
    return rest_send_json(rest, channel_id, request);
}

char* rest_send_embed(discord_rest* rest, const char* channel_id, const char* title, const char* description, const int color_hex){
    char request[6144];
    snprintf(request, 6144,
        "{\"embeds\":[{\"title\":\"%s\",\"description\":\"%s\",\"color\":%d}]}",
        title,
        description,
        color_hex
    );
    return rest_send_json(rest, channel_id, request);
}