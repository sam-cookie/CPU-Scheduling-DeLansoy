#ifndef READY_QUEUE_H
#define READY_QUEUE_H

#include "process.h"

/* ── ready_queue.h ──────────────────────────────────────────────────
 * circular buffer of Process* pointers, used by fcfs/sjf/stcf/rr.
 * grows automatically if it fills up so we never silently drop a process.
 * ─────────────────────────────────────────────────────────────────── */

typedef struct {
    Process **queue;
    int head;
    int tail;
    int size;
    int capacity;
} ReadyQueue;

ReadyQueue *create_ready_queue(int capacity);
void free_ready_queue(ReadyQueue *q);
int enqueue(ReadyQueue *q, Process *p);  /* returns 1 on success, 0 on failure */
Process *dequeue(ReadyQueue *q);
Process *peek(ReadyQueue *q);
int queue_is_empty(ReadyQueue *q);

#endif /* READY_QUEUE_H */
