#ifndef HELP_H
#define HELP_H

#include "discord/discord_rest.h"

char* help_command(discord_rest* rest, const char* channel_id);

char* help_helldivers(discord_rest* rest, const char* channel_id);

char* help_info(discord_rest* rest, const char* channel_id);

#endif