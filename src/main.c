#include "discord.h"
#include "cJSON.h"

int main(){
    https_ctx_init();

    struct discord_bot bot;
    if(discord_init(&bot)){
        https_ctx_free();
        return 1;
    }

    while(1){
        struct ws_message* message = ws_receive(&bot.ws);
        if(!message){
            break;
        }
        cJSON* mjson = cJSON_ParseWithLength(message->payload, message->payload_len);
        cJSON* t = cJSON_GetObjectItemCaseSensitive(mjson, "t");
        if(t && t->valuestring && strcmp(t->valuestring, "MESSAGE_CREATE") == 0){
            cJSON* d = cJSON_GetObjectItemCaseSensitive(mjson, "d");
            cJSON* content = cJSON_GetObjectItemCaseSensitive(d, "content");
            char* msg_content = content->valuestring;
            char* channel_id = cJSON_GetObjectItemCaseSensitive(d, "channel_id")->valuestring;
            if(!msg_content){
                cJSON_Delete(mjson);
                ws_free_message(message);
                continue;
            }
            if(strcmp(msg_content, "!kys") == 0){
                discord_send_message(&bot, channel_id, "Go fuck yourself.");
            }
        }
        cJSON_Delete(mjson);
        ws_free_message(message);
    }

    discord_cleanup(&bot);
    https_ctx_free();
    return 0;
}