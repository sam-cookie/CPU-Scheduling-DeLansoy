#ifndef OUTPUT_UTILS_H
#define OUTPUT_UTILS_H

#include "scheduler.h"
#include "mlfq_config_utils.h"

/* ── output.h ────────────────────────────────────────────────────────
 * output and analysis helpers for scheduler results.
 * displays gantt charts, metrics, analysis, and convoy effect detection.
 * ─────────────────────────────────────────────────────────────────── */

void print_results(SchedulerState *state, const char *label);
void mlfq_print_analysis(Process *procs, int n, MLFQConfig *config);
void mlfq_print_phase_summary(Process *procs, int n, int current_time);
void check_convoy_effect(const Process *procs, int n);

#endif /* OUTPUT_UTILS_H */
