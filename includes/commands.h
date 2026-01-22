#ifndef COMMANDS_H
#define COMMANDS_H

#include "discord.h"

void handle_command(struct discord_bot* bot, const char* channel_id, const char* content);

#endif