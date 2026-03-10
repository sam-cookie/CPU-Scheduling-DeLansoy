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

/* MLFQ config helpers (implemented in mlfq.c) */
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

#endif /* SCHEDULER_H */