#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scheduler.h"
#include "metrics.h"
#include "gantt.h"

/* ═══════════════════════════════════════════════════════════════════
 * utils.c — shared helpers used across all schedulers
 *
 * sections:
 *   1. ready queue       (used by fcfs, sjf, stcf, rr)
 *   2. index queue       (used by mlfq — stores int indices)
 *   3. mlfq runtime      (queue management, boost, demotion)
 *   4. mlfq config       (load from file, defaults, free)
 *   5. output helpers    (print_results, analysis, convoy check)
 *   6. compare runner    (runs all algorithms side by side)
 * ═══════════════════════════════════════════════════════════════════ */

/* ── 1. ready queue ─────────────────────────────────────────────────
 * circular buffer of Process* pointers, used by fcfs/sjf/stcf/rr.
 * grows automatically if it fills up so we never silently drop a process.
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

void enqueue(ReadyQueue *q, Process *p) {
    if (q->size >= q->capacity) {
        /* double capacity and fix up the circular buffer if it wrapped */
        int new_cap          = q->capacity * 2;
        Process **new_queue  = realloc(q->queue, new_cap * sizeof(Process *));
        if (!new_queue) { fprintf(stderr, "enqueue: realloc failed\n"); return; }
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

/* ── 2. index queue ─────────────────────────────────────────────────
 * stores int indices into procs[] instead of process copies.
 * same idea as ReadyQueue but lighter — just ints, no circular buffer.
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

void iq_enqueue(IdxQueue *q, int idx) {
    /* grow if full */
    if (q->size >= q->capacity) {
        q->capacity *= 2;
        int *new_indices = realloc(q->indices, q->capacity * sizeof(int));
        if (!new_indices) {
            fprintf(stderr, "iq_enqueue: realloc failed\n");
            return;  /* cannot grow, silently drop this enqueue */
        }
        q->indices = new_indices;
    }
    q->indices[q->size++] = idx;
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

/*
 * mlfq_create — set up mlfq with queues and time tracking
 * config: queue settings, n: number of processes
 * returns: new mlfq struct or null on error
 */
MLFQState *mlfq_create(MLFQConfig *config, int n) {
    MLFQState *mlfq          = calloc(1, sizeof(MLFQState));
    mlfq->num_queues    = config->num_levels;
    mlfq->boost_period  = config->boost_period;
    mlfq->last_boost    = 0;
    mlfq->queues        = calloc(config->num_levels, sizeof(IdxQueue *));
    mlfq->time_in_queue = calloc(n, sizeof(int));
    for (int i = 0; i < config->num_levels; i++)
        mlfq->queues[i] = iq_create(n + 1);
    return mlfq;
}

/*
 * mlfq_destroy — clean up mlfq memory
 * mlfq: the struct to free, num_levels: number of queues
 */
void mlfq_destroy(MLFQState *mlfq, int num_levels) {
    for (int i = 0; i < num_levels; i++)
        iq_free(mlfq->queues[i]);
    free(mlfq->queues);
    free(mlfq->time_in_queue);
    free(mlfq);
}

/* add a process to a specific queue level and announce it */
/*
 * mlfq_enqueue_process — add process to a specific queue level
 * mlfq: the scheduler, procs: process array, idx: process index
 * level: queue to add to, t: current time
 * slice_start: start of current slice (for mid-quantum check)
 * current_pid: pid of currently running process (for mid-quantum)
 */
void mlfq_enqueue_process(MLFQState *mlfq, Process *procs,
                           int idx, int level, int t, int slice_start, const char *current_pid) {
    if (current_pid && t > slice_start) {
        printf("t=%-3d:   Process %s arrives mid-quantum; %s continues\n",
               t, procs[idx].pid, current_pid);
    } else {
        printf("t=%-3d:   Process %s arrives → enters Q%d\n",
               t, procs[idx].pid, level);
    }
    procs[idx].priority      = level;
    mlfq->time_in_queue[idx] = 0;
    iq_enqueue(mlfq->queues[level], idx);
}

/*
 * mlfq_highest_nonempty — find highest priority queue with processes
 * mlfq: the scheduler
 * returns: queue index (0 highest) or -1 if all empty
 */
int mlfq_highest_nonempty(MLFQState *mlfq) {
    for (int i = 0; i < mlfq->num_queues; i++)
        if (!iq_is_empty(mlfq->queues[i]))
            return i;
    return -1;
}

/*
 * mlfq_boost — move all processes back to q0 if boost time reached
 * mlfq: the scheduler, procs: process array, current_time: now
 * returns: 1 if boosted, 0 otherwise
 */
int mlfq_boost(MLFQState *mlfq, Process *procs, int current_time) {
    if (current_time - mlfq->last_boost < mlfq->boost_period) return 0;
    printf("t=%-3d: Priority boost: all processes → Q0 (allotments reset)\n",
           current_time);
    for (int i = 1; i < mlfq->num_queues; i++) {
        while (!iq_is_empty(mlfq->queues[i])) {
            int idx;
            if (iq_dequeue(mlfq->queues[i], &idx)) {
                procs[idx].priority  = 0;
                mlfq->time_in_queue[idx] = 0;
                iq_enqueue(mlfq->queues[0], idx);
            }
        }
    }
    mlfq->last_boost = current_time;
    return 1;
}

/*
 * mlfq_requeue_or_demote — after slice, demote if allotment used, else requeue
 * mlfq: the scheduler, idx: process index, procs: process array, config: settings
 */
void mlfq_requeue_or_demote(MLFQState *mlfq, int idx,
                              Process *procs, MLFQConfig *config) {
    int lvl       = procs[idx].priority;
    int allotment = config->levels[lvl].allotment;
    int last_q    = mlfq->num_queues - 1;

    /* allotment=-1 means infinite (lowest queue), never demote */
    if (allotment != -1 && mlfq->time_in_queue[idx] >= allotment
        && lvl < last_q) {
        printf("  [MLFQ] Process %s demoted Q%d → Q%d "
               "(used %d/%d allotment)\n",
               procs[idx].pid, lvl, lvl + 1,
               mlfq->time_in_queue[idx], allotment);
        procs[idx].priority++;
        mlfq->time_in_queue[idx] = 0;
    }
    iq_enqueue(mlfq->queues[procs[idx].priority], idx);
}

/* ── 4. mlfq config ─────────────────────────────────────────────────
 * load from file, return defaults, free when done
 * ─────────────────────────────────────────────────────────────────── */

/*
 * load_mlfq_config — read mlfq settings from file
 * path: file path
 * returns: config struct or null on error
 */
MLFQConfig *load_mlfq_config(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) { perror(path); return NULL; }

    MLFQConfig *cfg   = calloc(1, sizeof(MLFQConfig));
    cfg->levels       = calloc(16, sizeof(MLFQLevel));
    cfg->boost_period = 200;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        int qid, quantum, allotment;
        if (sscanf(line, "Q%d %d %d", &qid, &quantum, &allotment) == 3) {
            if (qid < 16) {
                /* validate quantum: must be positive or -1 (FCFS mode) */
                if (quantum != -1 && quantum <= 0) {
                    fprintf(stderr, "Error: MLFQ config Q%d has invalid quantum %d (must be > 0 or -1 for FCFS)\n", qid, quantum);
                    fclose(f);
                    free_mlfq_config(cfg);
                    return NULL;
                }
                /* validate allotment: must be positive or -1 (infinite) */
                if (allotment != -1 && allotment <= 0) {
                    fprintf(stderr, "Error: MLFQ config Q%d has invalid allotment %d (must be > 0 or -1 for infinite)\n", qid, allotment);
                    fclose(f);
                    free_mlfq_config(cfg);
                    return NULL;
                }
                cfg->levels[qid].level        = qid;
                cfg->levels[qid].time_quantum  = quantum;
                cfg->levels[qid].allotment     = allotment;
                if (qid + 1 > cfg->num_levels)
                    cfg->num_levels = qid + 1;
            }
        } else {
            int period;
            if (sscanf(line, "BOOST_PERIOD %d", &period) == 1) {
                /* validate boost_period: must be positive */
                if (period <= 0) {
                    fprintf(stderr, "Error: BOOST_PERIOD must be positive, got %d\n", period);
                    fclose(f);
                    free_mlfq_config(cfg);
                    return NULL;
                }
                cfg->boost_period = period;
            }
        }
    }
    fclose(f);
    return cfg;
}

/*
 * default_mlfq_config — create default mlfq settings
 * returns: config with standard values
 */
MLFQConfig *default_mlfq_config(void) {
    MLFQConfig *cfg   = calloc(1, sizeof(MLFQConfig));
    cfg->num_levels   = 3;
    cfg->boost_period = 200;
    cfg->levels       = calloc(3, sizeof(MLFQLevel));
    /* q0: short jobs, q1: medium, q2: long batch (fcfs) */
    cfg->levels[0] = (MLFQLevel){ .level=0, .time_quantum=10, .allotment=50  };
    cfg->levels[1] = (MLFQLevel){ .level=1, .time_quantum=30, .allotment=150 };
    cfg->levels[2] = (MLFQLevel){ .level=2, .time_quantum=-1, .allotment=-1  };
    return cfg;
}

/*
 * free_mlfq_config — clean up config memory
 * cfg: the config to free
 */
void free_mlfq_config(MLFQConfig *cfg) {
    if (!cfg) return;
    free(cfg->levels);
    free(cfg);
}

/*
 * mlfq_print_config — show the current mlfq settings
 * config: the config to display
 */
void mlfq_print_config(MLFQConfig *config) {
    printf("=== MLFQ Configuration ===\n");
    for (int i = 0; i < config->num_levels; i++) {
        MLFQLevel *lvl = &config->levels[i];
        if (lvl->time_quantum == -1) {
            if (i == config->num_levels - 1)
                printf("Queue %d: FCFS                 (lowest priority)\n", i);
            else
                printf("Queue %d: FCFS\n", i);
        } else {
            if (i == 0)
                printf("Queue %d: q=%d,  allotment=%d  (highest priority)\n",
                       i, lvl->time_quantum, lvl->allotment);
            else
                printf("Queue %d: q=%d,  allotment=%d\n",
                       i, lvl->time_quantum, lvl->allotment);
        }
    }
    printf("Boost period: %d\n\n", config->boost_period);
}

/* short vs long analysis — fused with convoy check since both are
 * post-simulation observations about how processes behaved */

/*
 * mlfq_print_phase_summary — show processes and round-robin order after boost
 * procs: process array, n: number of processes, current_time: now
 */
void mlfq_print_phase_summary(Process *procs, int n, int current_time) {
    printf("t=%d: ", current_time);
    int first = 1;
    for (int i = 0; i < n; i++) {
        if (!procs[i].completed) {
            if (!first) printf(", ");
            printf("%s(%dms left)", procs[i].pid, procs[i].remaining_time);
            first = 0;
        }
    }
    printf(" all in Q0\n");
    printf("       Round-robin: each runs 10ms/turn, order ");
    first = 1;
    for (int i = 0; i < n; i++) {
        if (!procs[i].completed) {
            if (!first) printf("→");
            printf("%s", procs[i].pid);
            first = 0;
        }
    }
    printf("\n");
    // Placeholder for the [list]
    printf("       [A@%d, B@%d, C@%d, ...]\n", current_time, current_time + 10, current_time + 20);
}

/*
 * mlfq_print_analysis — show if processes are short or long jobs
 * procs: process array, n: number of processes, config: settings
 */
void mlfq_print_analysis(Process *procs, int n, MLFQConfig *config) {
    printf("=== Analysis ===\n");
    for (int i = 0; i < n; i++) {
        if (procs[i].total_cpu_time <= config->levels[0].allotment)
            printf("  %s: short job — completed within Q0 allotment\n",
                   procs[i].pid);
        else
            printf("  %s: long job  — used %d total cpu ticks\n",
                   procs[i].pid, procs[i].total_cpu_time);
    }
}

/* convoy effect check — only relevant for fcfs where a long job
 * can block everyone behind it for a long time */
void check_convoy_effect(const Process *procs, int n) {
    for (int i = 0; i < n; i++)
        if (procs[i].waiting_time > procs[i].burst_time)
            printf("Convoy effect detected: Process %s waited %d time units\n",
                   procs[i].pid, procs[i].waiting_time);
}

/* print gantt + metrics for any algorithm.
 * fcfs also gets the convoy check since it's the one prone to it. */
void print_results(SchedulerState *state, const char *label) {
    gantt_print(state);

    Metrics m;
    calculate_metrics(state->processes, state->num_processes, &m);
    m.context_switches = state->context_switches;
    print_metrics(state->processes, state->num_processes, &m);

    if (state->context_switches > 0)
        printf("\nTotal context switches: %d\n", state->context_switches);

    if (strcmp(label, "FCFS") == 0)
        check_convoy_effect(state->processes, state->num_processes);
}

/* ── 6. compare runner ──────────────────────────────────────────────
 * runs all algorithms on the same workload and prints a summary table.
 * each algorithm gets a fresh clone of the process array so they
 * don't interfere with each other.
 * ─────────────────────────────────────────────────────────────────── */

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
        Process *clone = malloc((size_t)num_processes * sizeof(Process));    \
        if (!clone) break;                                                   \
        memcpy(clone, processes, (size_t)num_processes * sizeof(Process));   \
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