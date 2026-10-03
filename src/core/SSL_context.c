#include "core/SSL_context.h"

SSL_CTX* ssl_ctx = NULL;

int ssl_context_create(void){
    if(!ssl_ctx){
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
        ssl_ctx = SSL_CTX_new(TLS_client_method());
        if(!ssl_ctx){
            fputs("Failed to initialize SSL context\n", stderr);
            return -1;
        }
        SSL_CTX_set_session_cache_mode(ssl_ctx, SSL_SESS_CACHE_CLIENT);
    }
    return 0;
}

void ssl_context_free(void){
    if(ssl_ctx){
        SSL_CTX_free(ssl_ctx);
        ssl_ctx = NULL;
    }
}