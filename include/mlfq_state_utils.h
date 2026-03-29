#ifndef MLFQ_STATE_UTILS_H
#define MLFQ_STATE_UTILS_H

#include "process.h"
#include "index_queue_utils.h"
#include "mlfq_config_utils.h"

/* ── mlfq_state.h ───────────────────────────────────────────────────
 * runtime state management for MLFQ scheduler.
 * handles queue operations, priority boosts, and demotion logic.
 * ─────────────────────────────────────────────────────────────────── */

/* mlfq runtime state */
typedef struct {
    IdxQueue **queues;
    int        num_queues;
    int        boost_period;
    int        last_boost;
    int       *time_in_queue;
} MLFQState;

MLFQState *mlfq_create(MLFQConfig *config, int n);
void  mlfq_destroy(MLFQState *mlfq, int num_levels);
void  mlfq_enqueue_process(MLFQState *mlfq, Process *procs, int idx, int level, 
                           int t, int slice_start, const char *current_pid);
int   mlfq_highest_nonempty(MLFQState *mlfq);
int   mlfq_boost(MLFQState *mlfq, Process *procs, int current_time);
void  mlfq_requeue_or_demote(MLFQState *mlfq, int idx, 
                             Process *procs, MLFQConfig *config);

#endif /* MLFQ_STATE_UTILS_H */
