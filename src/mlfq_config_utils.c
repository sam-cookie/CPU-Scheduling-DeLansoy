#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mlfq_config_utils.h"

/* ── mlfq_config.c ──────────────────────────────────────────────────
 * implementation of MLFQ configuration loading and management.
 * ─────────────────────────────────────────────────────────────────── */

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

void free_mlfq_config(MLFQConfig *cfg) {
    if (!cfg) return;
    free(cfg->levels);
    free(cfg);
}

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
