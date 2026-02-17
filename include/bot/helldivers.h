#ifndef HELLDIVERS_H
#define HELLDIVERS_H

#include "core/discord.h"

//list of all the planets
extern const char* planet_names[267];

//list of all the faction (planet owners)
extern const char* factions[4];

extern const char* cyberstan_names[8];
extern const char* cyberstan_classes[8];
extern const int cyberstan_max[8];

char* helldivers_war_summary(struct discord_bot* bot, const char* channel_id);

char* helldivers_major_order(struct discord_bot* bot, const char* channel_id);

char* helldivers_cyberstan(struct discord_bot* bot, const char* channel_id);

#endif