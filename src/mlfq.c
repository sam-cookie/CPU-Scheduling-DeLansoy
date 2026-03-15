#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "process.h"
#include "scheduler.h"
#include "metrics.h"
#include "gantt.h"
#include "utils.c"

/*
 * mlfq.c — multi-level feedback queue scheduler
 *
 * big rule: never touch burst_time for scheduling decisions.
 * we only use remaining_time to know when a process is done.
 * all queue decisions come from time_in_queue vs allotment.
 *
 * all the messy helpers live in utils.c — this file is just
 * the simulation loop itself.
/* ─────────────────────────────────────────
 * schedule_mlfq — the simulation loop
 * ───────────────────────────────────────── */
int schedule_mlfq(SchedulerState *state, MLFQConfig *config) {
    printf("Running MLFQ Scheduler...\n\n");

    int n          = state->num_processes;
    Process *procs = state->processes;

    /* show what config we're using */
    mlfq_print_config(config);

    /* reset all process fields before we start */
    for (int i = 0; i < n; i++) {
        procs[i].remaining_time = procs[i].burst_time;
        procs[i].priority       = 0;
        procs[i].time_in_queue  = 0;
        procs[i].total_cpu_time = 0;
        procs[i].started        = 0;
        procs[i].completed      = 0;
        procs[i].start_time     = -1;
    }

    /* sort by arrival time so we can scan arrivals linearly */
    for (int i = 1; i < n; i++) {
        Process tmp = procs[i];
        int j = i - 1;
        while (j >= 0 && procs[j].arrival_time > tmp.arrival_time) {
            procs[j+1] = procs[j];
            j--;
        }
        procs[j+1] = tmp;
    }

    /* spin up the mlfq — queues + allotment counters */
    MLFQ *mlfq = mlfq_create(config, n);
    if (!mlfq) return -1;

    int t              = 0;   /* simulation clock                  */
    int completed      = 0;   /* how many processes finished       */
    int next_to_arrive = 0;   /* index into sorted procs[]         */

    printf("=== Execution Trace ===\n");

    while (completed < n) {

        /* add any processes that arrived by now — always goes to Q0 */
        while (next_to_arrive < n &&
               procs[next_to_arrive].arrival_time <= t) {
            mlfq_enqueue_process(mlfq, procs, next_to_arrive, 0, t);
            next_to_arrive++;
        }

        /* boost all processes to Q0 if enough time has passed */
        if (mlfq->boost_period > 0)
            mlfq_boost(mlfq, procs, t);

        /* find the highest priority queue that has something in it */
        int qi = mlfq_highest_nonempty(mlfq);

        /* nothing ready — jump clock forward to the next arrival */
        if (qi == -1) {
            if (next_to_arrive < n)
                t = procs[next_to_arrive].arrival_time;
            else
                break;
            continue;
        }

        /* grab the next process from that queue by index */
        int idx    = iq_dequeue(mlfq->queues[qi]);
        Process *p = &procs[idx];

        /* safety — skip if already done (can happen after a boost) */
        if (p->completed) continue;

        /* note when this process first touched the cpu (for RT) */
        if (!p->started) {
            p->start_time = t;
            p->started    = 1;
        }

        /* how long does this process run this turn?
         * quantum=-1 means fcfs — run straight to completion */
        int quantum = config->levels[qi].time_quantum;
        int slice   = (quantum == -1)
                      ? p->remaining_time
                      : (p->remaining_time < quantum
                         ? p->remaining_time : quantum);

        int slice_start = t;

        /* run the slice tick by tick so mid-slice arrivals
         * can enter Q0 without waiting until the next turn */
        for (int tick = 0; tick < slice; tick++) {
            p->remaining_time--;
            mlfq->time_in_queue[idx]++;
            p->total_cpu_time++;
            t++;

            /* catch any arrivals that sneak in during this slice */
            while (next_to_arrive < n &&
                   procs[next_to_arrive].arrival_time <= t) {
                mlfq_enqueue_process(mlfq, procs, next_to_arrive, 0, t);
                next_to_arrive++;
            }
        }

        /* log this slice in the gantt chart */
        gantt_record(state, p->pid, slice_start, t);

        /* finished or not — decide what happens next */
        if (p->remaining_time == 0) {
            p->finish_time = t;
            p->completed   = 1;
            completed++;
            printf("  [t=%d] Process %s completed (Q%d)\n",
                   t, p->pid, p->priority);
        } else {
            /* still has work — demote if allotment burned, else re-enqueue */
            mlfq_requeue_or_demote(mlfq, idx, procs, config);
        }
    }

    /* tear down the queues */
    mlfq_destroy(mlfq, config->num_levels);

    /* mlfq-specific analysis — main.c handles gantt + metrics via print_results */
    printf("\n");
    mlfq_print_analysis(procs, n, config);

    return 0;
}