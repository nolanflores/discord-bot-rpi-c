#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H

#include "core/yyjson.h"
#include <pthread.h>
#include <stdint.h>

typedef enum event_type{
    UNDEFINED_EVENT,
    MESSAGE_CREATE,
    INTERACTION_CREATE,
    CLOSE_CONNECTION
} event_type;

typedef struct event_node{
    struct event_node* next; /* was `event_node* next` — invalid: the typedef
                               * name isn't in scope yet inside its own definition */
    yyjson_doc* json;
    yyjson_val* data;
    event_type type;
} event_node;

typedef struct event_queue{
    event_node* head;
    event_node* tail;
    int size;

    pthread_mutex_t lock;
    pthread_cond_t  not_empty;
    int shutdown; /* set by event_queue_shutdown(); wakes blocked poppers */
} event_queue;

void event_queue_init(event_queue* queue);

/*
 * Parses json_string (one full gateway frame, any opcode) exactly once.
 * Always reports the frame's opcode via *out_op.
 *
 * If the opcode is 0 (Dispatch) and "t" is a recognized event type, also
 * reports the sequence number via *out_sequence (if non-NULL) and appends
 * a node to the queue, waking a blocked popper. For any other opcode, or
 * an unrecognized Dispatch "t", nothing is enqueued and *out_sequence is
 * left untouched — only read it when *out_op == 0.
 *
 * Ownership: on success, a Dispatch frame that gets enqueued hands its
 * parsed yyjson_doc to the queue; every other case frees the doc
 * internally. The caller never frees the doc either way.
 *
 * json_string must be null-terminated. out_op must be non-NULL;
 * out_sequence may be NULL if the caller doesn't need it.
 *
 * Returns 0 if the frame was parsed successfully, -1 on parse failure
 * (out params are not written in that case).
 */
int event_queue_push(event_queue* queue, const char* json_string, int* out_op, int64_t* out_sequence);

/*
 * Blocks until a node is available, then removes it and copies its fields
 * into *out. The caller becomes responsible for yyjson_doc_free(out->json)
 * once done with it.
 *
 * Returns 0 on success, -1 if the queue has been shut down and is empty
 * (no further events will ever arrive — stop calling this).
 */
int event_queue_pop(event_queue* queue, event_node* out);

/* Marks the queue as shutting down and wakes any thread blocked in
 * event_queue_pop(). Safe to call once the producer side is done pushing. */
void event_queue_shutdown(event_queue* queue);

/* Frees any remaining nodes (and their yyjson_docs) and destroys the lock
 * and condvar. Only call after all producer/consumer threads have exited. */
void event_queue_destroy(event_queue* queue);

#endif