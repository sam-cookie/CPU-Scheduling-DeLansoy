#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compare_utils.h"
#include "metrics.h"
#include "process.h"
#include "scheduler.h"

/* ── 6. compare runner ──────────────────────────────────────────────
 * runs all algorithms on the same workload and prints a summary table.
 * each algorithm gets a fresh clone of the process array with clean state
 * so they don't interfere with each other. computed fields are reset to
 * initial values for each run to ensure reliable comparisons.
 * ─────────────────────────────────────────────────────────────────── */

/* clone_processes — create independent copies with clean state */
static Process *clone_processes(const Process *src, int n) {
    Process *clone = malloc((size_t)n * sizeof(Process));
    if (!clone) return NULL;

    for (int i = 0; i < n; i++) {
        /* copy input fields */
        strcpy(clone[i].pid, src[i].pid);
        clone[i].arrival_time = src[i].arrival_time;
        clone[i].burst_time   = src[i].burst_time;

        /* reset computed fields to initial state */
        clone[i].remaining_time   = src[i].burst_time;  /* start with full burst time */
        clone[i].start_time       = -1;                 /* not started yet */
        clone[i].finish_time      = -1;                 /* not finished yet */
        clone[i].waiting_time     = 0;                  /* no waiting yet */
        clone[i].turnaround_time  = 0;                  /* no turnaround yet */

        /* reset MLFQ-specific fields */
        clone[i].priority         = 0;                  /* start at highest priority */
        clone[i].time_in_queue    = 0;                  /* no time in queue yet */
        clone[i].total_cpu_time   = 0;                  /* no CPU time used yet */

        /* reset state flags */
        clone[i].started          = 0;                  /* not started */
        clone[i].completed        = 0;                  /* not completed */
    }

    return clone;
}

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

/* clone the process array, run the algorithm, grab metrics, clean up */
#define RUN_ALGO(label, algo_call)                                           \
    do {                                                                     \
        Process *clone = clone_processes(processes, num_processes);          \
        if (!clone) break;                                                   \
        SchedulerState *st = calloc(1, sizeof(SchedulerState));              \
        if (!st) { free(clone); break; }                                     \
        st->processes     = clone;                                           \
        st->num_processes = num_processes;                                   \
        int rc = algo_call;                                                  \
        if (rc == 0) {                                                       \
            rows[nrows].name = (label);                                      \
            rows[nrows].cs   = st->context_switches;                         \
            rows[nrows].ok   = 1;                                            \
            calculate_metrics(clone, num_processes, &rows[nrows].m);         \
            nrows++;                                                         \
        }                                                                    \
        free(clone);                                                         \
        free(st);                                                            \
    } while (0)

    RUN_ALGO("FCFS", schedule_fcfs(st));
    RUN_ALGO("SJF",  schedule_sjf(st));
    RUN_ALGO("STCF", schedule_stcf(st));
    RUN_ALGO("RR",   schedule_rr(st, rr_quantum));
    RUN_ALGO("MLFQ", schedule_mlfq(st, mlfq_config));

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