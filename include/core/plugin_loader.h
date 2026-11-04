#ifndef CORE_PLUGIN_LOADER_H
#define CORE_PLUGIN_LOADER_H

#include "bot/plugin.h"

int plugin_loader_load(const char* directory);

int plugin_loader_handle_command(
    discord_rest* rest,
    const char* channel_id,
    int hash,
    const char* args
);

void plugin_loader_unload(void);

#endif
