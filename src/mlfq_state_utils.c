#include <stdio.h>
#include <stdlib.h>
#include "mlfq_state_utils.h"
#include "index_queue_utils.h"

/* ── mlfq_state.c ───────────────────────────────────────────────────
 * implementation of MLFQ runtime state management.
 * ─────────────────────────────────────────────────────────────────── */

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

void mlfq_destroy(MLFQState *mlfq, int num_levels) {
    for (int i = 0; i < num_levels; i++)
        iq_free(mlfq->queues[i]);
    free(mlfq->queues);
    free(mlfq->time_in_queue);
    free(mlfq);
}

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
    if (!iq_enqueue(mlfq->queues[level], idx)) {
        fprintf(stderr, "Error: failed to enqueue process %s to MLFQ queue %d\n", 
                procs[idx].pid, level);
    }
}

int mlfq_highest_nonempty(MLFQState *mlfq) {
    for (int i = 0; i < mlfq->num_queues; i++)
        if (!iq_is_empty(mlfq->queues[i]))
            return i;
    return -1;
}

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
                if (!iq_enqueue(mlfq->queues[0], idx)) {
                    fprintf(stderr, "Error: failed to enqueue process %s during boost\n", 
                            procs[idx].pid);
                }
            }
        }
    }
    mlfq->last_boost = current_time;
    return 1;
}

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
    if (!iq_enqueue(mlfq->queues[procs[idx].priority], idx)) {
        fprintf(stderr, "Error: failed to requeue/demote process %s\n", 
                procs[idx].pid);
    }
}
