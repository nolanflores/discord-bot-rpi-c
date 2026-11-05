#include "bot/help.h"

char* help_command(discord_rest* rest, const char* channel_id){
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
    return rest_send_json(rest, channel_id, help_payload);
}