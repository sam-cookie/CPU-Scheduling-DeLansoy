#ifndef INDEX_QUEUE_H
#define INDEX_QUEUE_H

/* ── index_queue.h ──────────────────────────────────────────────────
 * stores int indices into a process array instead of process copies.
 * used by mlfq to track which processes are in which priority queues.
 * ─────────────────────────────────────────────────────────────────── */

typedef struct {
    int *indices;
    int  size;
    int  capacity;
} IdxQueue;

IdxQueue *iq_create(int capacity);
void      iq_free(IdxQueue *q);
int       iq_enqueue(IdxQueue *q, int idx);  /* returns 1 on success, 0 on failure */
int       iq_dequeue(IdxQueue *q, int *out_idx);  /* returns 1 on success, 0 if empty */
int       iq_is_empty(IdxQueue *q);

#endif /* INDEX_QUEUE_H */
