#include <stdio.h>
#include <string.h>
#include "output_utils.h"
#include "metrics.h"
#include "gantt.h"

/* ── output.c ────────────────────────────────────────────────────────
 * implementation of output and analysis helpers for scheduler results.
 * ─────────────────────────────────────────────────────────────────── */

void mlfq_print_phase_summary(Process *procs, int n, int current_time) {
    printf("t=%d: ", current_time);
    int first = 1;
    for (int i = 0; i < n; i++) {
        if (!procs[i].completed) {
            if (!first) printf(", ");
            printf("%s(%dms left)", procs[i].pid, procs[i].remaining_time);
            first = 0;
        }
    }
    printf(" all in Q0\n");
    printf("       Round-robin: each runs 10ms/turn, order ");
    first = 1;
    for (int i = 0; i < n; i++) {
        if (!procs[i].completed) {
            if (!first) printf("→");
            printf("%s", procs[i].pid);
            first = 0;
        }
    }
    printf("\n");
    // Placeholder for the [list]
    printf("       [A@%d, B@%d, C@%d, ...]\n", current_time, current_time + 10, current_time + 20);
}

void mlfq_print_analysis(Process *procs, int n, MLFQConfig *config) {
    printf("=== Analysis ===\n");
    for (int i = 0; i < n; i++) {
        if (procs[i].total_cpu_time <= config->levels[0].allotment)
            printf("  %s: short job — completed within Q0 allotment\n",
                   procs[i].pid);
        else
            printf("  %s: long job  — used %d total cpu ticks\n",
                   procs[i].pid, procs[i].total_cpu_time);
    }
}

void check_convoy_effect(const Process *procs, int n) {
    for (int i = 0; i < n; i++)
        if (procs[i].waiting_time > procs[i].burst_time)
            printf("Convoy effect detected: Process %s waited %d time units\n",
                   procs[i].pid, procs[i].waiting_time);
}

void print_results(SchedulerState *state, const char *label) {
    gantt_print(state);

    Metrics m;
    calculate_metrics(state->processes, state->num_processes, &m);
    m.context_switches = state->context_switches;
    print_metrics(state->processes, state->num_processes, &m);

    if (state->context_switches > 0)
        printf("\nTotal context switches: %d\n", state->context_switches);

    if (strcmp(label, "FCFS") == 0)
        check_convoy_effect(state->processes, state->num_processes);
}
