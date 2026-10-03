#ifndef SSL_CONTEXT_H
#define SSL_CONTEXT_H

#include <openssl/ssl.h>

/*
 * The global SSL context.
 * This context is used for all TLS connections and should be initialized once at the start of the program.
 * It is created by calling ssl_context_create() and freed by calling ssl_context_free().
*/
extern SSL_CTX* ssl_ctx;

/*
 * Initializes the global SSL context.
 * This function must be called before any other https functions.
 * ssl_context_free should be called when the context is no longer needed.
 * 
 * Returns 0 on success, -1 on failure.
 */
int ssl_context_create(void);

/*
 * Frees the global SSL context.
 * This function should be called when the SSL context is no longer needed.
 */
void ssl_context_free(void);

#endif