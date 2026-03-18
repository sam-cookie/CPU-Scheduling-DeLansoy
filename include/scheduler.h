#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"

//schdulers and compare identifiers
typedef enum {
    ALGO_FCFS,
    ALGO_SJF,
    ALGO_STCF,
    ALGO_RR,
    ALGO_MLFQ,
    ALGO_COMPARE   /* Run all algorithms and compare */
} SchedulingAlgorithm;

//round robin config
typedef struct {
    int quantum;    /* Time slice (default: 30) */
} RRConfig;

//mlfq-specific structs
typedef struct {
    int level;          /* Queue priority level (0 = highest)   */
    int time_quantum;   /* Time slice for this queue (-1 = FCFS) */
    int allotment;      /* Max time before demotion (-1 = infinite) */
    Process *queue;     /* Array of processes in this queue      */
    int size;           /* Current number of processes in queue  */
} MLFQQueue;

typedef struct {
    MLFQQueue *queues;  /* Array of queues                       */
    int num_queues;     /* Number of priority levels             */
    int boost_period;   /* Period for priority boost (S)         */
    int last_boost;     /* Last time a boost occurred            */
} MLFQScheduler;

typedef struct {
    int level;          /* Queue index (0 = highest priority)    */
    int time_quantum;   /* Time slice; -1 means FCFS             */
    int allotment;      /* Max CPU time before demotion          */
} MLFQLevel;

//main configuraiton of a MLFQ
typedef struct {
    MLFQLevel *levels;  /* Array of level configurations         */
    int num_levels;     /* Number of queues                      */
    int boost_period;   /* How often to boost all processes → Q0 */
} MLFQConfig;

/* ── add these to scheduler.h ── */

/* index queue — used internally by mlfq */
typedef struct {
    int *indices;
    int  size;
    int  capacity;
} IdxQueue;

/* mlfq runtime state */
typedef struct {
    IdxQueue **queues;
    int        num_queues;
    int        boost_period;
    int        last_boost;
    int       *time_in_queue;
} MLFQ;

/* index queue helpers */
IdxQueue *iq_create(int capacity);
void      iq_free(IdxQueue *q);
void      iq_enqueue(IdxQueue *q, int idx);
int       iq_dequeue(IdxQueue *q);
int       iq_is_empty(IdxQueue *q);

/* mlfq utils */
MLFQ *mlfq_create(MLFQConfig *config, int n);
void  mlfq_destroy(MLFQ *mlfq, int num_levels);
void  mlfq_enqueue_process(MLFQ *mlfq, Process *procs, int idx, int level, int t, int slice_start, const char *current_pid);
int   mlfq_highest_nonempty(MLFQ *mlfq);
int   mlfq_boost(MLFQ *mlfq, Process *procs, int current_time);
void  mlfq_requeue_or_demote(MLFQ *mlfq, int idx, Process *procs, MLFQConfig *config);
void  mlfq_print_config(MLFQConfig *config);
void  mlfq_print_analysis(Process *procs, int n, MLFQConfig *config);
void  mlfq_print_phase_summary(Process *procs, int n, int current_time);
MLFQConfig *load_mlfq_config(const char *path);
MLFQConfig *default_mlfq_config(void);
void        free_mlfq_config(MLFQConfig *cfg);

int schedule_fcfs(SchedulerState *state);
int schedule_sjf(SchedulerState *state);
int schedule_stcf(SchedulerState *state);
int schedule_rr(SchedulerState *state, int quantum);
int schedule_mlfq(SchedulerState *state, MLFQConfig *config);

//run all schedulers and compare
int schedule_compare(Process *processes, int num_processes,
                     int rr_quantum, MLFQConfig *mlfq_config);


//ready_queue implementation (used to determine which processes to run next, usually the one with the shortest burst time or the one that arrived first)
typedef struct {
    Process **queue;
    int head;
    int tail;
    int size;
    int capacity;
} ReadyQueue;

ReadyQueue *create_ready_queue(int capacity);
void free_ready_queue(ReadyQueue *q);
void enqueue(ReadyQueue *q, Process *p);
Process *dequeue(ReadyQueue *q);
Process *peek(ReadyQueue *q);
int queue_is_empty(ReadyQueue *q);
void print_results(SchedulerState *state, const char *label);
void check_convoy_effect(const Process *procs, int n);

#endif /* SCHEDULER_H */