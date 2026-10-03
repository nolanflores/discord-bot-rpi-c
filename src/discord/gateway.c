#include "discord/gateway.h"
#include "discord_token.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


static void* event_loop(void* arg);
static int send_heartbeat(gateway* gw);

/* ------------------------------------------------------------------- */

static int send_heartbeat(gateway* gw){
    int64_t seq = atomic_load(&gw->last_sequence);
    char heartbeat_payload[64];
    snprintf(heartbeat_payload, 64,"{\"op\":1,\"d\":%d}", seq);
    if(ws_send(&gw->ws, heartbeat_payload) != 0){
        fputs("Failed to send Heartbeat\n", stderr);
        return -1;
    }
    return 0;
}

/* ------------------------------------------------------------------- */

static void* heartbeat_loop(void* arg){
    gateway* gw = (gateway*)arg;
    fputs("Starting Heartbeat Thread\n", stdout);

    char buffer[8192];
    int n = ws_receive(&gw->ws, buffer, sizeof(buffer));
    if(n <= 0){
        fputs("Failed to receive Hello frame\n", stderr);
        atomic_store(&gw->close_bool, 1);
        return NULL;
    }

    yyjson_doc* hello_doc = yyjson_read(buffer, (size_t)n, 0);
    if(!hello_doc){
        fputs("Failed to parse Hello frame\n", stderr);
        atomic_store(&gw->close_bool, 1);
        return NULL;
    }
    yyjson_val* hello_root = yyjson_doc_get_root(hello_doc);
    yyjson_val* hello_d = yyjson_obj_get(hello_root, "d");
    int interval_ms = yyjson_get_int(yyjson_obj_get(hello_d, "heartbeat_interval"));
    yyjson_doc_free(hello_doc);

    if(interval_ms <= 0){
        fputs("Invalid heartbeat_interval in Hello\n", stderr);
        atomic_store(&gw->close_bool, 1);
        return NULL;
    }
    atomic_store(&gw->heartbeat_interval, interval_ms);

    char identify_payload[512];
    snprintf(identify_payload, 512,
        "{"
            "\"op\":2,"
            "\"d\":{"
                "\"token\":\"%s\","
                "\"intents\":%d,"
                "\"properties\":{"
                    "\"os\":\"linux\","
                    "\"browser\":\"rpi_bot\","
                    "\"device\":\"rpi_bot\""
                "}"
            "}"
        "}",
        DISCORD_TOKEN,
        (1 << 9) | (1 << 12) | (1 << 15)//GUILD_MESSAGES, DIRECT_MESSAGES, MESSAGE_CONTENT
    );
    if(ws_send(&gw->ws, identify_payload)){
        fputs("Failed to send Identify\n", stderr);
        atomic_store(&gw->close_bool, 1);
        return NULL;
    }

    if (pthread_create(&gw->event_thread, NULL, event_loop, gw)) {
        fputs("Failed to create event thread\n", stderr);
        atomic_store(&gw->close_bool, 1);
        return NULL;
    }
    atomic_store(&gw->event_thread_started, 1);

    atomic_store(&gw->ack_bool, 1);
    int wait_ms = (int)(interval_ms * ((double)rand() / (double)RAND_MAX));

    pthread_mutex_lock(&gw->close_mutex);
    while(!atomic_load(&gw->close_bool)){
        struct timespec deadline;
        clock_gettime(0, &deadline);
        deadline.tv_sec  += wait_ms / 1000;
        deadline.tv_nsec += (long)(wait_ms % 1000) * 1000000L;
        if (deadline.tv_nsec >= 1000000000L) {
            deadline.tv_sec  += 1;
            deadline.tv_nsec -= 1000000000L;
        }

        int rc = pthread_cond_timedwait(&gw->close_cond, &gw->close_mutex, &deadline);

        if(atomic_load(&gw->close_bool)){
            break;
        }
        if(rc != ETIMEDOUT){
            continue;
        }

        if(!atomic_load(&gw->ack_bool)){
            fputs("Heartbeat ACK not received -- connection appears dead\n", stderr);
            atomic_store(&gw->close_bool, 1);
            break;
        }

        atomic_store(&gw->ack_bool, 0);

        pthread_mutex_unlock(&gw->close_mutex);
        int send_rc = send_heartbeat(gw);
        pthread_mutex_lock(&gw->close_mutex);

        if(send_rc != 0){
            fputs("Failed to send heartbeat\n", stderr);
            atomic_store(&gw->close_bool, 1);
            break;
        }

        wait_ms = interval_ms;
    }
    pthread_mutex_unlock(&gw->close_mutex);

    return NULL;
}

static void* event_loop(void* arg){
    gateway* gw = (gateway*)arg;
    char buffer[8192];

    while(!atomic_load(&gw->close_bool)){
        int n = ws_receive(&gw->ws, buffer, sizeof(buffer));
        if(n <= 0){
            if(!atomic_load(&gw->close_bool)){
                fputs("Gateway connection lost unexpectedly\n", stderr);
                atomic_store(&gw->close_bool, 1);
                event_queue_shutdown(&gw->event_queue);
            }
            break;
        }

        int op = -1;
        int64_t seq = atomic_load(&gw->last_sequence);
        if(event_queue_push(&gw->event_queue, buffer, &op, &seq) != 0){
            continue;
        }
 
        switch(op){
            case 0: /* Dispatch -- already enqueued (if recognized) by push() above */
                atomic_store(&gw->last_sequence, seq);
                break;
            case 1: /* Heartbeat request -- Discord wants an immediate beat */
                send_heartbeat(gw);
                break;
            case 7:  /* Reconnect */
            case 9:  /* Invalid Session */
                fputs("Gateway requested reconnect / invalid session\n", stderr);
                atomic_store(&gw->close_bool, 1);
                event_queue_shutdown(&gw->event_queue);
                break;
            case 11: /* Heartbeat ACK */
                atomic_store(&gw->ack_bool, 1);
                break;
            default:
                break;
        }
    }
 
    return NULL;
}

/* ------------------------------------------------------------------- */

int gateway_connect(gateway* gw){
    if (ws_connect(&gw->ws, "gateway.discord.gg", "443")) {
        return -1;
    }
    if (pthread_mutex_init(&gw->close_mutex, NULL)) {
        fputs("Failed to initialize close mutex\n", stderr);
        ws_close(&gw->ws);
        return -1;
    }
    if (pthread_cond_init(&gw->close_cond, NULL)) {
        fputs("Failed to initialize close condition\n", stderr);
        pthread_mutex_destroy(&gw->close_mutex);
        ws_close(&gw->ws);
        return -1;
    }

    event_queue_init(&gw->event_queue);

    atomic_store(&gw->close_bool, 0);
    atomic_store(&gw->ack_bool, 0);
    atomic_store(&gw->last_sequence, 0);
    atomic_store(&gw->heartbeat_interval, 0);
    atomic_store(&gw->event_thread_started, 0);

    if (pthread_create(&gw->heartbeat_thread, NULL, heartbeat_loop, gw)) {
        fputs("Failed to create heartbeat thread\n", stderr);
        event_queue_destroy(&gw->event_queue);
        pthread_cond_destroy(&gw->close_cond);
        pthread_mutex_destroy(&gw->close_mutex);
        ws_close(&gw->ws);
        return -1;
    }

    return 0;
}

int gateway_receive(gateway* gw, event_node* out){
    return event_queue_pop(&gw->event_queue, out);
}

void gateway_close(gateway* gw){
    pthread_mutex_lock(&gw->close_mutex);
    atomic_store(&gw->close_bool, 1);
    pthread_cond_signal(&gw->close_cond); /* wake heartbeat_thread immediately */
    pthread_mutex_unlock(&gw->close_mutex);

    ws_close(&gw->ws); /* unblocks event_thread's blocking ws_receive */

    pthread_join(gw->heartbeat_thread, NULL);
    if (atomic_load(&gw->event_thread_started)) {
        pthread_join(gw->event_thread, NULL);
    }

    /* Wake anything blocked in gateway_receive() so it observes shutdown
     * instead of blocking forever -- safe now that both producer threads
     * have been joined. */
    event_queue_shutdown(&gw->event_queue);
    event_queue_destroy(&gw->event_queue);

    pthread_cond_destroy(&gw->close_cond);
    pthread_mutex_destroy(&gw->close_mutex);
}