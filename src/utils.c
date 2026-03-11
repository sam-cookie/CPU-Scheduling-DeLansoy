#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scheduler.h"
#include "metrics.h"

// for rr and mlfq fr fr 
ReadyQueue *create_ready_queue(int capacity) {
    ReadyQueue *q = calloc(1, sizeof(ReadyQueue));
    if (!q) return NULL;
    q->queue    = calloc(capacity, sizeof(Process *));
    if (!q->queue) { free(q); return NULL; }
    q->capacity = capacity;
    q->size     = 0;
    q->head     = 0;
    q->tail     = 0;
    return q;
}

void free_ready_queue(ReadyQueue *q) {
    if (!q) return;
    free(q->queue);
    free(q);
}

Process *dequeue(ReadyQueue *q) {
    if (!q || q->size == 0) return NULL;
    Process *p = q->queue[q->head];
    q->head = (q->head + 1) % q->capacity;
    q->size--;
    return p;
}

Process *peek(ReadyQueue *q) {
    if (!q || q->size == 0) return NULL;
    return q->queue[q->head];
}

int queue_is_empty(ReadyQueue *q) {
    if (!q) return 1;
    return q->size == 0;
}

void enqueue(ReadyQueue *q, Process *p) {
    if (q->size >= q->capacity) {
        /* Grow the buffer by doubling */
        int new_cap = q->capacity * 2;
        Process **new_queue = realloc(q->queue, new_cap * sizeof(Process *));
        if (!new_queue) {
            fprintf(stderr, "Error: queue realloc failed\n");
            return;
        }
        /* If tail has wrapped around, we need to linearize the buffer */
        if (q->tail <= q->head) {
            int old_cap = q->capacity;
            for (int i = 0; i < q->tail; i++) {
                new_queue[old_cap + i] = new_queue[i];
            }
            q->tail = old_cap + q->tail;
        }
        q->queue    = new_queue;
        q->capacity = new_cap;
    }
    q->queue[q->tail] = p;
    q->tail = (q->tail + 1) % q->capacity;
    q->size++;
}


// schedule_compare — runs FCFS and SJF, prints a side-by-side table.
int schedule_compare(Process *processes, int num_processes,
                     int rr_quantum, MLFQConfig *mlfq_config) {
    (void)rr_quantum;
    (void)mlfq_config;

    typedef struct {
        const char *name;
        Metrics     m;
        int         cs;
        int         ok;
    } Row;

    Row rows[5];
    int nrows = 0;

#define RUN_ALGO(label, algo_call)                                          \
    do {                                                                    \
        Process *clone = malloc((size_t)num_processes * sizeof(Process));   \
        if (!clone) break;                                                  \
        memcpy(clone, processes, (size_t)num_processes * sizeof(Process));  \
        SchedulerState *st = calloc(1, sizeof(SchedulerState));             \
        if (!st) { free(clone); break; }                                    \
        st->processes     = clone;                                          \
        st->num_processes = num_processes;                                  \
        int rc = algo_call;                                                 \
        if (rc == 0) {                                                      \
            rows[nrows].name = (label);                                     \
            rows[nrows].cs   = st->context_switches;                        \
            rows[nrows].ok   = 1;                                           \
            calculate_metrics(clone, num_processes, &rows[nrows].m);        \
            nrows++;                                                        \
        }                                                                   \
        free(clone);                                                        \
        free(st);                                                           \
    } while (0)

    RUN_ALGO("FCFS", schedule_fcfs(st));
    RUN_ALGO("SJF",  schedule_sjf(st));
    // not yet implemented 
    // RUN_ALGO("STCF", schedule_stcf(st)); 
    // RUN_ALGO("RR",   schedule_rr(st, rr_quantum)); 
    // RUN_ALGO("MLFQ", schedule_mlfq(st, mlfq_config)); 

#undef RUN_ALGO

    printf("\n=== Algorithm Comparison ===\n\n");
    printf("%-10s | %8s | %8s | %8s | %8s\n",
           "Algorithm", "Avg TT", "Avg WT", "Avg RT", "Ctx SW");
    printf("-----------|----------|----------|----------|----------\n");

    for (int i = 0; i < nrows; i++) {
        if (!rows[i].ok) continue;
        printf("%-10s | %8.1f | %8.1f | %8.1f | %8d\n",
               rows[i].name,
               rows[i].m.avg_turnaround,
               rows[i].m.avg_waiting,
               rows[i].m.avg_response,
               rows[i].cs);
    }

    return 0;
}

// check convoy effect
void check_convoy_effect(const Process *procs, int n) {
    for (int i = 0; i < n; i++) {
        if (procs[i].waiting_time > procs[i].burst_time)
            printf("Convoy effect detected: Process %s waited %d time units\n",
                   procs[i].pid, procs[i].waiting_time);
    }
}
