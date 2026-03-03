#include "bot/help.h"

char* help_command(struct discord_bot* bot, const char* channel_id){
    char *help_payload = "{"
        "\"embeds\": [{"
            "\"title\": \"Command Help\","
            "\"description\": \"Use `!help <category>` for more info\","
            "\"color\": 3894651,"
            "\"fields\": ["
                "{"
                    "\"name\": \"GIFs\","
                    "\"value\": \"`!sell` `!buddy` `!sold` `!bust` `!slap` `!react` `!pray` `!whip` `!tap` `!nuke` `!notice` `!winner`\","
                    "\"inline\": false"
                "},{"
                    "\"name\": \"Helldivers 2\","
                    "\"value\": \"`!mo` `!war`\","
                    "\"inline\": false"
                "},{"
                    "\"name\": \"Bot Info\","
                    "\"value\": \"`!version` `!repo`\","
                    "\"inline\": false"
                "}"
            "],"
            "\"footer\": {\"text\": \"!help hd2 - !help info\"}"
        "}]"
    "}";
    return discord_send_raw(bot, channel_id, help_payload);
}

char* help_helldivers(struct discord_bot* bot, const char* channel_id){
    char *help_hd2_payload = "{"
        "\"embeds\": [{"
            "\"title\": \"Helldivers 2\","
            "\"color\": 3894651,"
            "\"fields\": ["
                "{"
                    "\"name\": \"`!mo`\","
                    "\"value\": \"Displays the currently active Major Orders\","
                    "\"inline\": false"
                "},{"
                    "\"name\": \"`!war`\","
                    "\"value\": \"Displays the current state of the galactic war\","
                    "\"inline\": false"
                "}"
            "],"
            "\"footer\": {\"text\": \"Data pulled live from the Helldivers 2 API\"}"
        "}]"
    "}";
    return discord_send_raw(bot, channel_id, help_hd2_payload);
}

char* help_info(struct discord_bot* bot, const char* channel_id){
    char *help_info_payload = "{"
        "\"embeds\": [{"
            "\"title\": \"Bot Info\","
            "\"color\": 3894651,"
            "\"fields\": ["
                "{"
                    "\"name\": \"`!version`\","
                    "\"value\": \"Prints the version of C the bot was compiled with\","
                    "\"inline\": false"
                "},{"
                    "\"name\": \"`!repo`\","
                    "\"value\": \"Links to the bot's source code repository\","
                    "\"inline\": false"
                "}"
            "]"
        "}]"
    "}";
    return discord_send_raw(bot, channel_id, help_info_payload);
}