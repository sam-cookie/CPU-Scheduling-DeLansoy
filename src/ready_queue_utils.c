#include <stdio.h>
#include <stdlib.h>
#include "ready_queue_utils.h"

/* ── ready_queue.c ──────────────────────────────────────────────────
 * implementation of circular buffer for Process* pointers.
 * ─────────────────────────────────────────────────────────────────── */

ReadyQueue *create_ready_queue(int capacity) {
    ReadyQueue *q = calloc(1, sizeof(ReadyQueue));
    if (!q) return NULL;
    q->queue    = calloc(capacity, sizeof(Process *));
    if (!q->queue) { free(q); return NULL; }
    q->capacity = capacity;
    return q;
}

void free_ready_queue(ReadyQueue *q) {
    if (!q) return;
    free(q->queue);
    free(q);
}

int enqueue(ReadyQueue *q, Process *p) {
    if (q->size >= q->capacity) {
        /* double capacity and fix up the circular buffer if it wrapped */
        int new_cap          = q->capacity * 2;
        Process **new_queue  = realloc(q->queue, new_cap * sizeof(Process *));
        if (!new_queue) { 
            fprintf(stderr, "enqueue: realloc failed\n"); 
            return 0;  /* failed to grow queue */
        }
        if (q->tail <= q->head) {
            /* tail wrapped around — copy the wrapped part to the new space */
            int old_cap = q->capacity;
            for (int i = 0; i < q->tail; i++)
                new_queue[old_cap + i] = q->queue[i];  /* copy from old buffer */
            q->tail = old_cap + q->tail;
        }
        q->queue    = new_queue;
        q->capacity = new_cap;
    }
    q->queue[q->tail] = p;
    q->tail = (q->tail + 1) % q->capacity;
    q->size++;
    return 1;  /* success */
}

Process *dequeue(ReadyQueue *q) {
    if (!q || q->size == 0) return NULL;
    Process *p = q->queue[q->head];
    q->head    = (q->head + 1) % q->capacity;
    q->size--;
    return p;
}

Process *peek(ReadyQueue *q) {
    if (!q || q->size == 0) return NULL;
    return q->queue[q->head];
}

int queue_is_empty(ReadyQueue *q) {
    return !q || q->size == 0;
}
