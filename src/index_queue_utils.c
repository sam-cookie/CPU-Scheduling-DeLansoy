#include <stdio.h>
#include <stdlib.h>
#include "index_queue_utils.h"

/* ── index_queue.c ──────────────────────────────────────────────────
 * implementation of index queue used by MLFQ scheduler.
 * ─────────────────────────────────────────────────────────────────── */

IdxQueue *iq_create(int capacity) {
    IdxQueue *q = calloc(1, sizeof(IdxQueue));
    q->indices  = calloc(capacity, sizeof(int));
    q->capacity = capacity;
    return q;
}

void iq_free(IdxQueue *q) {
    if (!q) return;
    free(q->indices);
    free(q);
}

int iq_enqueue(IdxQueue *q, int idx) {
    /* grow if full */
    if (q->size >= q->capacity) {
        q->capacity *= 2;
        int *new_indices = realloc(q->indices, q->capacity * sizeof(int));
        if (!new_indices) {
            fprintf(stderr, "iq_enqueue: realloc failed\n");
            return 0;  /* cannot grow, enqueue failed */
        }
        q->indices = new_indices;
    }
    q->indices[q->size++] = idx;
    return 1;  /* success */
}

int iq_dequeue(IdxQueue *q, int *out_idx) {
    if (q->size == 0) return 0;  /* empty */
    *out_idx = q->indices[0];
    /* shift left — fine for small n */
    for (int i = 1; i < q->size; i++)
        q->indices[i-1] = q->indices[i];
    q->size--;
    return 1;  /* success */
}

int iq_is_empty(IdxQueue *q) {
    return !q || q->size == 0;
}
