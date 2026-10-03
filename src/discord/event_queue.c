#include "discord/event_queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void event_queue_init(event_queue* queue){
    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
    queue->shutdown = 0;
    pthread_mutex_init(&queue->lock, NULL);
    pthread_cond_init(&queue->not_empty, NULL);
}

int event_queue_push(event_queue* queue, const char* json_string, int* out_op, int64_t* out_sequence){
    yyjson_doc* doc = yyjson_read(json_string, strlen(json_string), 0);
    if (!doc) {
        fputs("event_queue_push: failed to parse frame\n", stderr);
        return -1;
    }

    yyjson_val* root = yyjson_doc_get_root(doc);
    int op = yyjson_get_int(yyjson_obj_get(root, "op"));
    *out_op = op;

    if (op != 0) {
        /* Not a Dispatch frame -- nothing to enqueue, caller branches on *out_op. */
        yyjson_doc_free(doc);
        return 0;
    }

    yyjson_val* s_val = yyjson_obj_get(root, "s");
    if (out_sequence && s_val && !yyjson_is_null(s_val)) {
        *out_sequence = (int64_t)yyjson_get_int(s_val);
    }

    yyjson_val* d_val = yyjson_obj_get(root, "d");
    yyjson_val* t_val = yyjson_obj_get(root, "t");
    const char* t = yyjson_get_str(t_val);

    event_type type = UNDEFINED_EVENT;
    if (t) {
        if (strcmp(t, "MESSAGE_CREATE") == 0) type = MESSAGE_CREATE;
        else if (strcmp(t, "INTERACTION_CREATE") == 0) type = INTERACTION_CREATE;
    }

    if (type == UNDEFINED_EVENT) {
        yyjson_doc_free(doc); /* parsed fine, just not an event type we track */
        return 0;
    }

    event_node* node = malloc(sizeof(event_node));
    if (!node) {
        yyjson_doc_free(doc);
        return -1;
    }
    node->next = NULL;
    node->json = doc;
    node->data = d_val;
    node->type = type;

    pthread_mutex_lock(&queue->lock);
    if (queue->tail) {
        queue->tail->next = node;
    } else {
        queue->head = node;
    }
    queue->tail = node;
    queue->size++;
    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->lock);

    return 0;
}

int event_queue_pop(event_queue* queue, event_node* out){
    pthread_mutex_lock(&queue->lock);
    while (queue->head == NULL && !queue->shutdown) {
        pthread_cond_wait(&queue->not_empty, &queue->lock);
    }

    if (queue->head == NULL) { /* shut down and fully drained */
        pthread_mutex_unlock(&queue->lock);
        return -1;
    }

    event_node* node = queue->head;
    queue->head = node->next;
    if (queue->head == NULL) {
        queue->tail = NULL;
    }
    queue->size--;
    pthread_mutex_unlock(&queue->lock);

    *out = *node;
    out->next = NULL;
    free(node);
    return 0;
}

void event_queue_shutdown(event_queue* queue){
    pthread_mutex_lock(&queue->lock);
    queue->shutdown = 1;
    pthread_cond_broadcast(&queue->not_empty);
    pthread_mutex_unlock(&queue->lock);
}

void event_queue_destroy(event_queue* queue){
    event_node* node = queue->head;
    while (node) {
        event_node* next = node->next;
        if (node->json) {
            yyjson_doc_free(node->json);
        }
        free(node);
        node = next;
    }
    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;

    pthread_mutex_destroy(&queue->lock);
    pthread_cond_destroy(&queue->not_empty);
}