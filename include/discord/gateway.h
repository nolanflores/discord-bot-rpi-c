#ifndef GATEWAY_H
#define GATEWAY_H

#include "core/websocket.h"
#include "discord/event_queue.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>

typedef struct gateway{
    websocket ws;

    pthread_t heartbeat_thread;
    pthread_t event_thread;
    atomic_bool event_thread_started; //guards against joining an uncreated event_thread if heartbeat_loop dies before spawning it

    _Atomic int64_t last_sequence;      // last 's' seen from a Dispatch frame
    atomic_int      heartbeat_interval; // ms, set once after Hello, before event_thread exists
    atomic_bool     ack_bool;           // set by event_thread on opcode 11, cleared by heartbeat_thread
    atomic_bool     close_bool;         // shutdown flag, checked by both loops

    pthread_mutex_t close_mutex; // pairs with close_cond; also used for heartbeat_thread's interruptible sleep
    pthread_cond_t  close_cond; // condition variable for closing the gateway

    event_queue event_queue;
} gateway;

int gateway_connect(gateway* gw);

/* Blocks until an event is available. Returns 0 on success, -1 once the
 * gateway has shut down and no further events will arrive -- the caller
 * should stop calling this. */
int gateway_receive(gateway* gw, event_node* out);

void gateway_close(gateway* gw);

#endif