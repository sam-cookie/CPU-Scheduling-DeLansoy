#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"
#include "ready_queue_utils.h"
#include "index_queue_utils.h"
#include "mlfq_state_utils.h"
#include "mlfq_config_utils.h"
#include "output_utils.h"
#include "compare_utils.h"

//schedulers and compare identifiers
typedef enum {
    ALGO_FCFS,
    ALGO_SJF,
    ALGO_STCF,
    ALGO_RR,
    ALGO_MLFQ,
    ALGO_COMPARE   /* Run all algorithms and compare */
} SchedulingAlgorithm;

int schedule_fcfs(SchedulerState *state);
int schedule_sjf(SchedulerState *state);
int schedule_stcf(SchedulerState *state);
int schedule_rr(SchedulerState *state, int quantum);
int schedule_mlfq(SchedulerState *state, MLFQConfig *config);

#endif /* SCHEDULER_H */