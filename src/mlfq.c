#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "process.h"
#include "scheduler.h"
#include "metrics.h"
#include "gantt.h"

/*
 * mlfq.c — multi-level feedback queue scheduler
 *
 * ./schedsim --algorithm=MLFQ --input=tests/workload1.txt
 * ./schedsim --algorithm=MLFQ --processes="A:0:240,B:10:180,C:20:150"
 * all functions are placed in utils.c and are broken down into small sections
 * 
 * general flow:
 * 1. reset process states and sort by arrival time
 * 2. create mlfq queues and run simulation loop
 * 3. in each tick: add arrivals, priority boost check first ueue, run highest priority process
 * 4. after slice: check if process done, check if there is allotment left (if yes, then requeue, if no, then demote)
 * 5. print tracing, analysis, and gantt chart
 */


/*
 * schedule_mlfq — main simulation function for mlfq algorithm
 * sets up config, processes, runs the loop, prints output
 */
int schedule_mlfq(SchedulerState *state, MLFQConfig *config) {
    int n = state->num_processes;

    // work on local copy to avoid side effects
    Process *procs = malloc((size_t)n * sizeof(Process));
    if (!procs) return -1;
    memcpy(procs, state->processes, (size_t)n * sizeof(Process));

    /* show what config we're using */
    mlfq_print_config(config);

    printf("=== Execution Trace ===\n\n");

    int phase = 1;
    if (config->boost_period > 0) {
        printf("--- Phase %d: t=0 to t=%d (first boost period) ---\n\n",
               phase, config->boost_period);
    }

    /* reset all process fields before we start */
    for (int i = 0; i < n; i++) {
        procs[i].remaining_time = procs[i].burst_time;
        procs[i].priority       = 0;
        procs[i].time_in_queue  = 0;
        procs[i].total_cpu_time = 0;
        procs[i].started        = 0;
        procs[i].completed      = 0;
        procs[i].start_time     = -1;
        procs[i].waiting_time   = 0;
    }

    /* sort by arrival time so we can scan arrivals as they come*/
    for (int i = 1; i < n; i++) {
        Process tmp = procs[i];
        int j = i - 1;
        while (j >= 0 && procs[j].arrival_time > tmp.arrival_time) {
            procs[j+1] = procs[j];
            j--;
        }
        procs[j+1] = tmp;
    }

    /* create the mlfq — queues + allotment counters */
    MLFQState *mlfq = mlfq_create(config, n);
    if (!mlfq) return -1;

    int t              = 0;   /* simulation clock                  */
    int completed      = 0;   /* how many processes finished       */
    int next_to_arrive = 0;   /* index into sorted procs[]         */

    while (completed < n) {
        /* add any processes that arrived by now (automatically sa Q0) */
        while (next_to_arrive < n &&
               procs[next_to_arrive].arrival_time <= t) {
            mlfq_enqueue_process(mlfq, procs, next_to_arrive, 0, t, -1, NULL);
            next_to_arrive++;
        }

        /* boost all processes to Q0 if enough time has passed */
        if (mlfq->boost_period > 0) {
            if (mlfq_boost(mlfq, procs, t)) {
                phase++;
                printf("\n--- Phase %d: t=%d to t=%d (next boost period) ---\n\n",
                       phase, t, t + mlfq->boost_period);
                mlfq_print_phase_summary(procs, n, t);
            }
        }

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
        int idx;
        if (!iq_dequeue(mlfq->queues[qi], &idx)) {
            /* This shouldn't happen since we checked highest_nonempty */
            continue;
        }
        Process *p = &procs[idx];

        /* safety — skip if already done (can happen after a boost) */
        if (p->completed) continue;

        /* how long does this process run this turn?
         * quantum=-1 means fcfs — run straight to completion */
        int quantum = config->levels[qi].time_quantum;
        int slice   = (quantum == -1)
                      ? p->remaining_time
                      : (p->remaining_time < quantum
                         ? p->remaining_time : quantum);

        int slice_start = t;
        printf("t=%-3d:   Process %s runs (Q%d)\n",
               slice_start, p->pid, qi);

        /* note when this process first touched the cpu (for RT) */
        if (!p->started) {
            p->start_time = slice_start;
            p->started    = 1;
        }

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
                mlfq_enqueue_process(mlfq, procs, next_to_arrive, 0, t, slice_start, p->pid);
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
            int turnaround = p->finish_time - p->arrival_time;
            printf("t=%-3d:   Process %s COMPLETES at t=%d\n",
                   t, p->pid, t);
            printf("       (turnaround = %d - %d = %dms)\n",
                   t, p->arrival_time, turnaround);
        } else {
            /* still has work — demote if allotment burned, else re-enqueue */
            int old_lvl = p->priority;
            int used    = mlfq->time_in_queue[idx];

            mlfq_requeue_or_demote(mlfq, idx, procs, config);

            printf("t=%-3d:   Process %s preempted (Q%d allotment used: %dms)\n",
                   t, p->pid, old_lvl, used);
            if (p->priority != old_lvl) {
                printf("t=%-3d:   Process %s → Q%d (exhausted %dms Q%d allotment)\n",
                       t, p->pid, p->priority, used, old_lvl);
            }
        }
    }

    /* tear down the queues */
    mlfq_destroy(mlfq, config->num_levels);

    /* mlfq-specific analysis — main.c handles gantt + metrics via print_results */
    printf("\n");
    mlfq_print_analysis(procs, n, config);

    // copy results back to state->processes (matching by PID)
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            if (strcmp(state->processes[j].pid, procs[i].pid) == 0) {
                state->processes[j].start_time  = procs[i].start_time;
                state->processes[j].finish_time  = procs[i].finish_time;
                state->processes[j].waiting_time = procs[i].waiting_time;
                state->processes[j].started      = procs[i].started;
                state->processes[j].completed    = procs[i].completed;
                state->processes[j].priority     = procs[i].priority;
                state->processes[j].time_in_queue = procs[i].time_in_queue;
                state->processes[j].total_cpu_time = procs[i].total_cpu_time;
                break;
            }

    free(procs);
    return 0;
}