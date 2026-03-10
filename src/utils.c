#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scheduler.h"
#include "metrics.h"

// for rr and mlfq fr fr 
ReadyQueue *create_ready_queue(int capacity) { (void)capacity; return NULL; }
void        free_ready_queue(ReadyQueue *q)  { (void)q; }
void        enqueue(ReadyQueue *q, Process *p) { (void)q; (void)p; }
Process    *dequeue(ReadyQueue *q)           { (void)q; return NULL; }
Process    *peek(ReadyQueue *q)              { (void)q; return NULL; }
int         queue_is_empty(ReadyQueue *q)    { (void)q; return 1; }


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