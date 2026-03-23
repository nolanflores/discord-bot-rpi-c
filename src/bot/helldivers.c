#include "bot/helldivers.h"
#include "core/https_socket.h"
#include "core/yyjson.h"

static yyjson_doc* helldivers_request(const char* path){
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
    yyjson_doc* doc = yyjson_read(response, strlen(response), 0);
    free(response);
    return doc;
}

static int helldivers_double_request(const char* path1, const char* path2, yyjson_doc** result1, yyjson_doc** result2){
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
    *result1 = yyjson_read(response1, strlen(response1), 0);
    free(response1);
    *result2 = yyjson_read(response2, strlen(response2), 0);
    free(response2);
    return 0;
}

char* helldivers_war_summary(struct discord_bot* bot, const char* channel_id){
    yyjson_doc* status_doc = NULL;
    yyjson_doc* info_doc = NULL;
    if(helldivers_double_request("/api/WarSeason/801/Status", "/api/WarSeason/801/WarInfo", &status_doc, &info_doc))
        return NULL;
    if(status_doc == NULL || info_doc == NULL){
        yyjson_doc_free(status_doc);
        yyjson_doc_free(info_doc);
        return NULL;
    }
    yyjson_val* status = yyjson_doc_get_root(status_doc);
    yyjson_val* info = yyjson_doc_get_root(info_doc);
    int wartime = yyjson_get_int(yyjson_obj_get(status, "time"));
    yyjson_val* topPlanets[3];
    int playerCounts[3] = {0};
    int topIndexes[3] = {0};
    yyjson_val* planetStatus = yyjson_obj_get(status, "planetStatus");
    int size = yyjson_arr_size(planetStatus);
    for(int i = 0; i < size; i++){
        yyjson_val* planet = yyjson_arr_get(planetStatus, i);
        int players = yyjson_get_int(yyjson_obj_get(planet, "players"));
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
    yyjson_val* planetInfos = yyjson_obj_get(info, "planetInfos");
    char message[1024];
    int message_len = 0;
    for(int i = 2; i >= 0; i--){
        int owner = yyjson_get_int(yyjson_obj_get(topPlanets[i], "owner"));
        if(owner == 1){
            yyjson_val* planetEvents = yyjson_obj_get(status, "planetEvents");
            int eventSize = yyjson_arr_size(planetEvents);
            for(int j = 0; j < eventSize; j++){
                yyjson_val* event = yyjson_arr_get(planetEvents, j);
                int eventId = yyjson_get_int(yyjson_obj_get(event, "planetIndex"));
                if(eventId == topIndexes[i]){
                    int race = yyjson_get_int(yyjson_obj_get(event, "race"));
                    int health = yyjson_get_int(yyjson_obj_get(event, "health"));
                    int maxHealth = yyjson_get_int(yyjson_obj_get(event, "maxHealth"));
                    int expireTime = yyjson_get_int(yyjson_obj_get(event, "expireTime"));
                    int hours = (expireTime - wartime) / 3600;
                    int minutes = ((expireTime - wartime) % 3600) / 60;
                    float defendedPercent = 100.0f * (maxHealth - health) / maxHealth;
                    message_len += snprintf(message + message_len, 1024 - message_len, "**%s** | *%s*\\n%'d Current Divers\\n%.2f%% Defended\\n%dH %dM Remaining\\n\\n", planet_names[topIndexes[i]], factions[race-1], playerCounts[i], defendedPercent, hours, minutes);
                    break;
                }
            }
        }else{
            yyjson_val* planetInfo = yyjson_arr_get(planetInfos, topIndexes[i]);
            int maxHealth = yyjson_get_int(yyjson_obj_get(planetInfo, "maxHealth"));
            int health = yyjson_get_int(yyjson_obj_get(topPlanets[i], "health"));
            message_len += snprintf(message + message_len, 1024 - message_len, "**%s** | *%s*\\n%'d Current Divers\\n%.2f%% Liberated\\n\\n", planet_names[topIndexes[i]], factions[owner-1], playerCounts[i], 100.0 - ((100.0 * health) / maxHealth));
        }
    }
    yyjson_val* spaceStations = yyjson_obj_get(status, "spaceStations");//i am pretty sure effect id 1238 is orbital blockade
    if(yyjson_arr_size(spaceStations) > 0){
        yyjson_val* dss = yyjson_arr_get(spaceStations, 0);
        int dssLocation = yyjson_get_int(yyjson_obj_get(dss, "planetIndex"));
        yyjson_val* dssPlanet = yyjson_arr_get(planetStatus, dssLocation);
        int dssOwner = yyjson_get_int(yyjson_obj_get(dssPlanet, "owner"));
        int dsstime = yyjson_get_int(yyjson_obj_get(dss, "currentElectionEndWarTime"));
        int hours = (dsstime - wartime) / 3600;
        int minutes = ((dsstime - wartime) % 3600) / 60;
        message_len += snprintf(message + message_len, 1024 - message_len, "### Democracy Space Station\\n**%s** | *%s*\\nNext FTL Jump in %dH %dM", planet_names[dssLocation], factions[dssOwner-1], hours, minutes);
    }else{
        message_len += snprintf(message + message_len, 1024 - message_len, "### Democracy Space Station\\nDSS Not Currently Available");
    }
    char* response = discord_send_embed(bot, channel_id, "Active Planets", message, 0xFFE900);
    yyjson_doc_free(status_doc);
    yyjson_doc_free(info_doc);
    return response;
}

char* helldivers_major_order(struct discord_bot* bot, const char* channel_id){
    yyjson_doc* mo_doc = helldivers_request("/api/v2/Assignment/War/801");
    if(mo_doc == NULL)
        return NULL;
    yyjson_val* json = yyjson_doc_get_root(mo_doc);
    char message[2048];
    int message_len = 0;
    int size = yyjson_arr_size(json);
    if(size == 0){
        yyjson_doc_free(mo_doc);
        return discord_send_embed(bot, channel_id, "High Command Orders", "Awaiting Next Orders", 0x016AB5);
    }
    for(int i = 0; i < size; i++){
        yyjson_val* order = yyjson_arr_get(json, i);
        yyjson_val* progress = yyjson_obj_get(order, "progress");
        int expiresIn = yyjson_get_int(yyjson_obj_get(order, "expiresIn"));
        yyjson_val* setting = yyjson_obj_get(order, "setting");
        const char* title = yyjson_get_str(yyjson_obj_get(setting, "overrideTitle"));
        const char* brief = yyjson_get_str(yyjson_obj_get(setting, "overrideBrief"));
        yyjson_val* tasks = yyjson_obj_get(setting, "tasks");
        int taskCount = yyjson_arr_size(tasks);
        int targetCounts[16];
        int numTargets = 0;
        for(int j = 0; j < taskCount; j++){
            yyjson_val* task = yyjson_arr_get(tasks, j);
            yyjson_val* values = yyjson_obj_get(task, "values");
            yyjson_val* valueTypes = yyjson_obj_get(task, "valueTypes");
            int valueCount = yyjson_arr_size(values);
            for(int k = 0; k < valueCount; k++){
                if(yyjson_get_int(yyjson_arr_get(valueTypes, k)) == 3){
                    targetCounts[numTargets++] = yyjson_get_int(yyjson_arr_get(values, k));
                }
            }
        }
        int days = expiresIn / 86400;
        int hours = (expiresIn % 86400) / 3600;
        message_len += snprintf(message + message_len, 2048 - message_len, "**%s**\\n%s\\n\\n", title, brief);
        for(int j = 0; j < numTargets; j++){
            if(targetCounts[j] > 1){//No need to show 1/1 or 0/1
                int progressValue = yyjson_get_int(yyjson_arr_get(progress, j));
                message_len += snprintf(message + message_len, 2048 - message_len, "%'d / %'d (%.2f%%)\\n", progressValue, targetCounts[j], (100.0 * progressValue) / targetCounts[j]);
            }
        }
        message_len += snprintf(message + message_len, 2048 - message_len, "Expires in %d days and %d hours%s", days, hours, i < size - 1 ? "\\n\\n" : "");
    }
    char* response = discord_send_embed(bot, channel_id, "High Command Orders", message, 0x016AB5);
    yyjson_doc_free(mo_doc);
    return response;
}

char* helldivers_cyberstan(struct discord_bot* bot, const char* channel_id){
    yyjson_doc* cyber_doc = helldivers_request("/api/WarSeason/801/Status");
    if(cyber_doc == NULL)
        return NULL;
    yyjson_val* json = yyjson_doc_get_root(cyber_doc);
    char message[1024];
    int message_len = 0;
    yyjson_val* planetRegions = yyjson_obj_get(json, "planetRegions");
    int size = yyjson_arr_size(planetRegions);
    for(int i = 0; i < size; i++){
        yyjson_val* region = yyjson_arr_get(planetRegions, i);
        if(yyjson_get_int(yyjson_obj_get(region, "planetIndex")) != 260)
            continue;
        int regionIndex = yyjson_get_int(yyjson_obj_get(region, "regionIndex"));
        float health = 100.0f - ((100.0f * yyjson_get_int(yyjson_obj_get(region, "health"))) / cyberstan_max[regionIndex]);
        if(health == 100.0f)
            continue;
        int players = yyjson_get_int(yyjson_obj_get(region, "players"));
        message_len += snprintf(message + message_len, 1024 - message_len, "-# %s MegaFactory\\n**%s**\\n%.2f%% Controlled\\n%'d Current Divers\\n\\n", cyberstan_classes[regionIndex], cyberstan_names[regionIndex], health, players);
    }
    yyjson_val* globalResources = yyjson_obj_get(json, "globalResources");
    yyjson_val* reserves = yyjson_arr_get(globalResources, 0);
    int currentValue = yyjson_get_int(yyjson_obj_get(reserves, "currentValue"));
    int maxValue = yyjson_get_int(yyjson_obj_get(reserves, "maxValue"));
    int changePerHour = yyjson_get_int(yyjson_obj_get(reserves, "changePerSecond")) * 3600;
    snprintf(message + message_len, 1024 - message_len, "### %'d Forces in Reserve (%.2f%%)\\n-# %'d (%.2f%%) per hour", currentValue, (100.0 * currentValue) / maxValue, changePerHour, (100.0 * changePerHour) / maxValue);
    char* response = discord_send_embed(bot, channel_id, "Battle For Cyberstan", message, 0xFE6A67);
    yyjson_doc_free(cyber_doc);
    return response;
}