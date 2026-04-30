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

static int helldivers_double_request(const char* path1, const char* path2, cJSON** result1, cJSON** result2){
    struct https_socket sock;
    if(https_connect(&sock, "api.live.prod.thehelldiversgame.com", "443"))
        return 1;
    char request[512];
    snprintf(request, 512,
        "GET %s HTTP/1.1\r\n"
        "Host: api.live.prod.thehelldiversgame.com\r\n"
        "Accept-Language: en-US\r\n"
        "\r\n",
        path1
    );
    char* response1 = https_send(&sock, request);
    if(response1 == NULL){
        https_close(&sock);
        return 1;
    }
    snprintf(request, 512,
        "GET %s HTTP/1.1\r\n"
        "Host: api.live.prod.thehelldiversgame.com\r\n"
        "Accept-Language: en-US\r\n"
        "\r\n",
        path2
    );
    char* response2 = https_send(&sock, request);
    https_close(&sock);
    if(response2 == NULL){
        free(response1);
        return 1;
    }
    *result1 = cJSON_Parse(response1);
    free(response1);
    *result2 = cJSON_Parse(response2);
    free(response2);
    return 0;
}

char* helldivers_war_summary(struct discord_bot* bot, const char* channel_id){
    cJSON* status = NULL;
    cJSON* info = NULL;
    if(helldivers_double_request("/api/WarSeason/801/Status", "/api/WarSeason/801/WarInfo", &status, &info))
        return NULL;
    if(status == NULL || info == NULL){
        cJSON_Delete(status);
        cJSON_Delete(info);
        return NULL;
    }
    int wartime = cJSON_GetObjectItemCaseSensitive(status, "time")->valueint;
    cJSON* topPlanets[3];
    int playerCounts[3] = {0};
    int topIndexes[3] = {0};
    cJSON* planetStatus = cJSON_GetObjectItemCaseSensitive(status, "planetStatus");
    int size = cJSON_GetArraySize(planetStatus);
    for(int i = 0; i < size; i++){
        cJSON* planet = cJSON_GetArrayItem(planetStatus, i);
        int players = cJSON_GetObjectItemCaseSensitive(planet, "players")->valueint;
        if(playerCounts[0] < players){
            int index = 0;
            for(int j = 1; j < 3; j++){
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
    for(int i = 2; i >= 0; i--){
        int owner = cJSON_GetObjectItemCaseSensitive(topPlanets[i], "owner")->valueint;
        if(owner == 1){
            cJSON* planetEvents = cJSON_GetObjectItemCaseSensitive(status, "planetEvents");
            int eventSize = cJSON_GetArraySize(planetEvents);
            for(int j = 0; j < eventSize; j++){
                cJSON* event = cJSON_GetArrayItem(planetEvents, j);
                int eventId = cJSON_GetObjectItemCaseSensitive(event, "planetIndex")->valueint;
                if(eventId == topIndexes[i]){
                    int race = cJSON_GetObjectItemCaseSensitive(event, "race")->valueint;
                    int health = cJSON_GetObjectItemCaseSensitive(event, "health")->valueint;
                    int maxHealth = cJSON_GetObjectItemCaseSensitive(event, "maxHealth")->valueint;
                    int expireTime = cJSON_GetObjectItemCaseSensitive(event, "expireTime")->valueint;
                    int hours = (expireTime - wartime) / 3600;
                    int minutes = ((expireTime - wartime) % 3600) / 60;
                    float defendedPercent = 100.0f * (maxHealth - health) / maxHealth;
                    message_len += snprintf(message + message_len, 1024 - message_len, "**%s** | *%s*\\n%'d Current Divers\\n%.2f%% Defended\\n%dH %dM Remaining\\n\\n", planet_names[topIndexes[i]], factions[race-1], playerCounts[i], defendedPercent, hours, minutes);
                    break;
                }
            }
        }else{
            cJSON* planetInfo = cJSON_GetArrayItem(planetInfos, topIndexes[i]);
            int maxHealth = cJSON_GetObjectItemCaseSensitive(planetInfo, "maxHealth")->valueint;
            int health = cJSON_GetObjectItemCaseSensitive(topPlanets[i], "health")->valueint;
            message_len += snprintf(message + message_len, 1024 - message_len, "**%s** | *%s*\\n%'d Current Divers\\n%.2f%% Liberated\\n\\n", planet_names[topIndexes[i]], factions[owner-1], playerCounts[i], 100.0 - ((100.0 * health) / maxHealth));
        }
    }
    cJSON* spaceStations = cJSON_GetObjectItemCaseSensitive(status, "spaceStations");//i am pretty sure effect id 1238 is orbital blockade
    if(cJSON_GetArraySize(spaceStations) > 0){
        cJSON* dss = cJSON_GetArrayItem(spaceStations, 0);
        int dssLocation = cJSON_GetObjectItemCaseSensitive(dss, "planetIndex")->valueint;
        cJSON* dssPlanet = cJSON_GetArrayItem(planetStatus, dssLocation);
        int dssOwner = cJSON_GetObjectItemCaseSensitive(dssPlanet, "owner")->valueint;
        int dsstime = cJSON_GetObjectItemCaseSensitive(dss, "currentElectionEndWarTime")->valueint;
        int hours = (dsstime - wartime) / 3600;
        int minutes = ((dsstime - wartime) % 3600) / 60;
        message_len += snprintf(message + message_len, 1024 - message_len, "### Democracy Space Station\\n**%s** | *%s*\\nNext FTL Jump in %dH %dM", planet_names[dssLocation], factions[dssOwner-1], hours, minutes);
    }else{
        message_len += snprintf(message + message_len, 1024 - message_len, "### Democracy Space Station\\nDSS Not Currently Available");
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
    char message[2048];
    int message_len = 0;
    int size = cJSON_GetArraySize(json);
    if(size == 0){
        cJSON_Delete(json);
        return discord_send_embed(bot, channel_id, "High Command Orders", "Awaiting Next Orders", 0x016AB5);
    }
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
        message_len += snprintf(message + message_len, 2048 - message_len, "**%s**\\n%s\\n\\n", title, brief);
        for(int j = 0; j < numTargets; j++){
            if(targetCounts[j] > 1){//No need to show 1/1 or 0/1
                int progressValue = cJSON_GetArrayItem(progress, j)->valueint;
                message_len += snprintf(message + message_len, 2048 - message_len, "%'d / %'d (%.2f%%)\\n", progressValue, targetCounts[j], (100.0 * progressValue) / targetCounts[j]);
            }
        }
        message_len += snprintf(message + message_len, 2048 - message_len, "Expires in %d days and %d hours%s", days, hours, i < size - 1 ? "\\n\\n" : "");
    }
    char* response = discord_send_embed(bot, channel_id, "High Command Orders", message, 0x016AB5);
    cJSON_Delete(json);
    return response;
}

char* helldivers_cyberstan(struct discord_bot* bot, const char* channel_id){
    cJSON* json = helldivers_request("/api/WarSeason/801/Status");
    if(json == NULL)
        return NULL;
    char message[1024];
    int message_len = 0;
    cJSON* planetRegions = cJSON_GetObjectItemCaseSensitive(json, "planetRegions");
    int size = cJSON_GetArraySize(planetRegions);
    for(int i = 0; i < size; i++){
        cJSON* region = cJSON_GetArrayItem(planetRegions, i);
        if(cJSON_GetObjectItemCaseSensitive(region, "planetIndex")->valueint != 260)
            continue;
        int regionIndex = cJSON_GetObjectItemCaseSensitive(region, "regionIndex")->valueint;
        float health = 100.0f - ((100.0f * cJSON_GetObjectItemCaseSensitive(region, "health")->valueint) / cyberstan_max[regionIndex]);
        if(health == 100.0f)
            continue;
        int players = cJSON_GetObjectItemCaseSensitive(region, "players")->valueint;
        message_len += snprintf(message + message_len, 1024 - message_len, "-# %s MegaFactory\\n**%s**\\n%.2f%% Controlled\\n%'d Current Divers\\n\\n", cyberstan_classes[regionIndex], cyberstan_names[regionIndex], health, players);
    }
    cJSON* globalResources = cJSON_GetObjectItemCaseSensitive(json, "globalResources");
    cJSON* reserves = cJSON_GetArrayItem(globalResources, 0);
    int currentValue = cJSON_GetObjectItemCaseSensitive(reserves, "currentValue")->valueint;
    int maxValue = cJSON_GetObjectItemCaseSensitive(reserves, "maxValue")->valueint;
    int changePerHour = cJSON_GetObjectItemCaseSensitive(reserves, "changePerSecond")->valueint * 3600;
    snprintf(message + message_len, 1024 - message_len, "### %'d Forces in Reserve (%.2f%%)\\n-# %'d (%.2f%%) per hour", currentValue, (100.0 * currentValue) / maxValue, changePerHour, (100.0 * changePerHour) / maxValue);
    char* response = discord_send_embed(bot, channel_id, "Battle For Cyberstan", message, 0xFE6A67);
    cJSON_Delete(json);
    return response;
}