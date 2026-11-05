#ifndef PLUGIN_DIRECTORY
#error "PLUGIN_DIRECTORY is not defined"
#endif

#include "core/plugin_loader.h"
#include <dirent.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct loaded_plugin{
    void* handle;
    const bot_plugin* plugin;
}loaded_plugin;

static loaded_plugin* loaded_plugins;
static size_t loaded_plugin_count;

int plugin_loader_load(void){
    DIR* dir = opendir(PLUGIN_DIRECTORY);
    if(dir == NULL){
        fprintf(stderr, "Unable to open plugin directory '%s'\n", PLUGIN_DIRECTORY);
        return 1;
    }

    struct dirent* entry;
    int result = 0;
    while((entry = readdir(dir)) != NULL){
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char path[4096];
        int length = snprintf(path, sizeof(path), "%s/%s", PLUGIN_DIRECTORY, entry->d_name);
        if(length < 0 || (size_t)length >= sizeof(path)){
            fprintf(stderr, "Plugin path is too long: %s\n", entry->d_name);
            result = 1;
            continue;
        }

        void* handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
        if(handle == NULL){
            fprintf(stderr, "Unable to load plugin '%s': %s\n", path, dlerror());
            result = 1;
            continue;
        }

        dlerror();
        bot_plugin_init_function init = (bot_plugin_init_function)dlsym(
            handle, "bot_plugin_init"
        );
        const char* error = dlerror();
        if(error != NULL || init == NULL){
            fprintf(stderr, "Plugin '%s' has no bot_plugin_init: %s\n", path,
                error != NULL ? error : "symbol is missing");
            dlclose(handle);
            result = 1;
            continue;
        }

        const bot_plugin* plugin = init();
        if(plugin == NULL || plugin->commands == NULL || plugin->command_count == 0){
            fprintf(stderr, "Plugin '%s' has an invalid descriptor\n", path);
            dlclose(handle);
            result = 1;
            continue;
        }

        loaded_plugin* plugins = realloc(
            loaded_plugins, (loaded_plugin_count + 1) * sizeof(*loaded_plugins)
        );
        if(plugins == NULL){
            fprintf(stderr, "Unable to register plugin '%s'\n", path);
            dlclose(handle);
            closedir(dir);
            return 1;
        }
        loaded_plugins = plugins;
        loaded_plugins[loaded_plugin_count++] = (loaded_plugin){handle, plugin};
    }
    closedir(dir);
    return result;
}

int plugin_loader_handle_command(discord_rest* rest, const char* channel_id, int hash, const char* args){
    for(size_t i = 0; i < loaded_plugin_count; ++i){
        const bot_plugin* plugin = loaded_plugins[i].plugin;
        for(size_t j = 0; j < plugin->command_count; ++j){
            const bot_command* registered = &plugin->commands[j];
            if(registered->handler != NULL && registered->hash == hash){
                char* response = registered->handler(rest, channel_id,
                    args != NULL ? args : "");
                if(response == NULL)
                    return 1;
                free(response);
                return 0;
            }
        }
    }
    return -1;
}

void plugin_loader_unload(void){
    for(size_t i = loaded_plugin_count; i > 0; --i)
        dlclose(loaded_plugins[i - 1].handle);
    free(loaded_plugins);
    loaded_plugins = NULL;
    loaded_plugin_count = 0;
}
