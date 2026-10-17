#ifndef TOKEN_PATH
#error "TOKEN_PATH is not defined"
#endif

#include "core/token.h"
#include <stdio.h>

static char token[128];
static int token_loaded = 0;

int load_token(void) {
    FILE* f = fopen(TOKEN_PATH, "r");
    if(!f)
        return -1;
    if(!fgets(token, sizeof(token), f)){
        fclose(f);
        return -1;
    }
    fclose(f);
    for(int i = 0; i < (int)sizeof(token); i++){
        char p = token[i];
        if(p == '\n' || p == '\r'){
            token[i] = '\0';
            break;
        }
    }
    token_loaded = 1;
    printf("Token loaded: %s\n", token);
    return 0;
}

const char* get_token(void){
    if(!token_loaded)
        load_token();
    return token;
}