#ifndef HELLDIVERS_H
#define HELLDIVERS_H

#include "core/discord.h"

//list of all the planets
extern const char* planet_names[267];

extern const char* factions[4];

int helldivers_init();

char* helldivers_war_summary(struct discord_bot* bot, const char* channel_id);

char* helldivers_major_order(struct discord_bot* bot, const char* channel_id);

void helldivers_free();

#endif