#ifndef COMMANDS_H
#define COMMANDS_H

#include "discord/discord_rest.h"

int handle_command(discord_rest* rest, const char* channel_id, const char* content);

#endif