#ifndef MLFQ_CONFIG_H
#define MLFQ_CONFIG_H

/* ── mlfq_config.h ──────────────────────────────────────────────────
 * configuration parsing and management for MLFQ scheduler.
 * load from file, create defaults, and cleanup.
 * ─────────────────────────────────────────────────────────────────── */

/* MLFQ configuration types */
typedef struct {
    int level;          /* Queue index (0 = highest priority)    */
    int time_quantum;   /* Time slice; -1 means FCFS             */
    int allotment;      /* Max CPU time before demotion          */
} MLFQLevel;

typedef struct {
    MLFQLevel *levels;  /* Array of level configurations         */
    int num_levels;     /* Number of queues                      */
    int boost_period;   /* How often to boost all processes → Q0 */
} MLFQConfig;

MLFQConfig *load_mlfq_config(const char *path);
MLFQConfig *default_mlfq_config(void);
void        free_mlfq_config(MLFQConfig *cfg);
void        mlfq_print_config(MLFQConfig *config);

#endif /* MLFQ_CONFIG_H */
