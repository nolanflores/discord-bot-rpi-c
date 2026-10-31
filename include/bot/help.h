#ifndef HELP_H
#define HELP_H

#include "discord/discord.h"

char* help_command(struct discord_bot* bot, const char* channel_id);

char* help_helldivers(struct discord_bot* bot, const char* channel_id);

char* help_info(struct discord_bot* bot, const char* channel_id);

#endif