#include "bot/helldivers.h"
#include "core/https_socket.h"

static cJSON* helldivers_request(const char* path){
    struct https_socket sock;
    if(https_connect(&sock, "api.live.prod.thehelldiversgame.com", "443")){
        return NULL;
    }
    char request[512];
    snprintf(request, 512,
        "GET %s HTTP/1.1\r\n"
        "Host: api.live.prod.thehelldiversgame.com\r\n"
        "Accept-Language: en-US\r\n"
        "\r\n",
        path
    );
    char* response = https_send(&sock, request);
    https_close(&sock);
    if(response == NULL)
        return NULL;
    cJSON* json = cJSON_Parse(response);
    free(response);
    return json;
}

char* helldivers_war_summary(struct discord_bot* bot, const char* channel_id){
    cJSON* status = helldivers_request("/api/WarSeason/801/Status");
    if(status == NULL){
        return NULL;
    }
    cJSON* info = helldivers_request("/api/WarSeason/801/WarInfo");
    if(info == NULL){
        cJSON_Delete(status);
        return NULL;
    }
    cJSON* topPlanets[5];
    int playerCounts[5] = {0};
    int topIndexes[5] = {0};
    cJSON* planetStatus = cJSON_GetObjectItemCaseSensitive(status, "planetStatus");
    int size = cJSON_GetArraySize(planetStatus);
    for(int i = 0; i < size; i++){
        cJSON* planet = cJSON_GetArrayItem(planetStatus, i);
        int players = cJSON_GetObjectItemCaseSensitive(planet, "players")->valueint;
        if(playerCounts[0] < players){
            int index = 0;
            for(int j = 1; j < 5; j++){
                if(playerCounts[j] < players)
                    index = j;
                else
                    break;
            }
            for(int j = 0; j < index; j++){
                topIndexes[j] = topIndexes[j + 1];
                topPlanets[j] = topPlanets[j + 1];
                playerCounts[j] = playerCounts[j + 1];
            }
            topIndexes[index] = i;
            topPlanets[index] = planet;
            playerCounts[index] = players;
        }
    }
    cJSON* planetInfos = cJSON_GetObjectItemCaseSensitive(info, "planetInfos");
    char message[1024];
    int message_len = 0;
    for(int i = 4; i >= 0; i--){
        cJSON* planetInfo = cJSON_GetArrayItem(planetInfos, topIndexes[i]);
        int maxHealth = cJSON_GetObjectItemCaseSensitive(planetInfo, "maxHealth")->valueint;
        int owner = cJSON_GetObjectItemCaseSensitive(topPlanets[i], "owner")->valueint;
        int health = cJSON_GetObjectItemCaseSensitive(topPlanets[i], "health")->valueint;
        message_len += snprintf(message + message_len, 1024 - message_len, "%s | %s\\n%d Current Divers\\n%.2f%% Liberated%s", planet_names[topIndexes[i]], factions[owner-1], playerCounts[i], 100.0 - ((100.0 * health) / maxHealth), i > 0 ? "\\n\\n" : "");
    }
    char* response = discord_send_embed(bot, channel_id, "Active Planets", message, 0xFFE900);
    cJSON_Delete(status);
    cJSON_Delete(info);
    return response;
}

char* helldivers_major_order(struct discord_bot* bot, const char* channel_id){
    cJSON* json = helldivers_request("/api/v2/Assignment/War/801");
    if(json == NULL)
        return NULL;
    int size = cJSON_GetArraySize(json);
    char message[2048];
    int message_len = 0;
    for(int i = 0; i < size; i++){
        cJSON* order = cJSON_GetArrayItem(json, i);
        cJSON* progress = cJSON_GetObjectItemCaseSensitive(order, "progress");
        int expiresIn = cJSON_GetObjectItemCaseSensitive(order, "expiresIn")->valueint;
        cJSON* setting = cJSON_GetObjectItemCaseSensitive(order, "setting");
        char* title = cJSON_GetObjectItemCaseSensitive(setting, "overrideTitle")->valuestring;
        char* brief = cJSON_GetObjectItemCaseSensitive(setting, "overrideBrief")->valuestring;
        cJSON* tasks = cJSON_GetObjectItemCaseSensitive(setting, "tasks");
        int taskCount = cJSON_GetArraySize(tasks);
        int targetCounts[16];
        int numTargets = 0;
        for(int j = 0; j < taskCount; j++){
            cJSON* task = cJSON_GetArrayItem(tasks, j);
            cJSON* values = cJSON_GetObjectItemCaseSensitive(task, "values");
            cJSON* valueTypes = cJSON_GetObjectItemCaseSensitive(task, "valueTypes");
            int valueCount = cJSON_GetArraySize(values);
            for(int k = 0; k < valueCount; k++){
                if(cJSON_GetArrayItem(valueTypes, k)->valueint == 3){
                    targetCounts[numTargets++] = cJSON_GetArrayItem(values, k)->valueint;
                }
            }
        }
        int days = expiresIn / 86400;
        int hours = (expiresIn % 86400) / 3600;
        message_len += snprintf(message + message_len, 2048 - message_len, "### %s\\n%s\\n\\n", title, brief);
        for(int j = 0; j < numTargets; j++){
            int progressValue = cJSON_GetArrayItem(progress, j)->valueint;
            message_len += snprintf(message + message_len, 2048 - message_len, "%d / %d (%.2f%%)\\n", progressValue, targetCounts[j], (100.0 * progressValue) / targetCounts[j]);
        }
        message_len += snprintf(message + message_len, 2048 - message_len, "Expires in %d days and %d hours%s", days, hours, i < size - 1 ? "\\n\\n" : "");
    }
    char* response = discord_send_embed(bot, channel_id, "High Command Orders", message, 0xFFE900);
    cJSON_Delete(json);
    return response;
}