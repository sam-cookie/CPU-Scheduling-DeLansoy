#include <stdio.h>
#include <stdlib.h>
#include "scheduler.h"

// placeholder for mlfq

MLFQConfig *load_mlfq_config(const char *path) {
    (void)path;
    fprintf(stderr, "MLFQ not yet implemented.\n");
    return NULL;
}

MLFQConfig *default_mlfq_config(void) {
    fprintf(stderr, "MLFQ not yet implemented.\n");
    return NULL;
}

void free_mlfq_config(MLFQConfig *cfg) {
    if (!cfg) return;
    free(cfg->levels);
    free(cfg);
}

int schedule_mlfq(SchedulerState *state, MLFQConfig *config) {
    (void)state; (void)config;
    fprintf(stderr, "MLFQ not yet implemented.\n");
    return -1;
}
