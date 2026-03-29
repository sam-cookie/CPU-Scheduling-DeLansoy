#ifndef COMPARE_UTILS_H
#define COMPARE_UTILS_H

#include "scheduler.h"
#include "mlfq_config_utils.h"

/* ── compare.h ───────────────────────────────────────────────────────
 * algorithm comparison runner.
 * runs all scheduling algorithms on the same workload and produces
 * a summary table of metrics for easy comparison.
 * ─────────────────────────────────────────────────────────────────── */

int schedule_compare(Process *processes, int num_processes,
                     int rr_quantum, MLFQConfig *mlfq_config);

#endif /* COMPARE_UTILS_H */
