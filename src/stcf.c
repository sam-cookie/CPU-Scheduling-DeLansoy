#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scheduler.h"
#include "gantt.h"

/*
 * STCF — Shortest Time-to-Completion First (preemptive SJF)
 *
 * Rules:
 *   1. At every tick, pick the arrived process with the shortest
 *      remaining_time.
 *   2. If a newly arrived process has a shorter remaining_time than
 *      the current one, preempt immediately.
 *   3. Track context switches whenever the running process changes.
 *   4. If no process is ready, advance the clock (CPU idle).
 */
int schedule_stcf(SchedulerState *state) {
    int n = state->num_processes;

    // work on local copy to avoid side effects
    Process *procs = malloc((size_t)n * sizeof(Process));
    if (!procs) return -1;
    memcpy(procs, state->processes, (size_t)n * sizeof(Process));

    for (int i = 0; i < n; i++) {
        procs[i].remaining_time = procs[i].burst_time;
        procs[i].completed      = 0;
        procs[i].started        = 0;
        procs[i].start_time     = -1;
        procs[i].waiting_time   = 0;
    }

    int  clock      = 0;
    int  completed  = 0;
    int  last_start = 0;
    char current_pid[MAX_PID_LEN] = "";

    while (completed < n) {

        Process *chosen = NULL;
        for (int i = 0; i < n; i++) {
            if (procs[i].completed || procs[i].arrival_time > clock) continue;
            if (chosen == NULL ||
                procs[i].remaining_time < chosen->remaining_time)
                chosen = &procs[i];
        }

        if (chosen == NULL) {
            clock++;
            continue;
        }

        if (strcmp(current_pid, chosen->pid) != 0) {
            if (strlen(current_pid) > 0) {
                gantt_record(state, current_pid, last_start, clock);
                state->context_switches++;
            }
            if (!chosen->started) {
                chosen->start_time = clock;
                chosen->started    = 1;
            }
            strcpy(current_pid, chosen->pid);
            last_start = clock;
        }

        chosen->remaining_time--;
        clock++;

        if (chosen->remaining_time == 0) {
            chosen->finish_time = clock;
            chosen->completed   = 1;

            // compute waiting_time before copying back
            chosen->waiting_time =
                (chosen->finish_time - chosen->arrival_time) - chosen->burst_time;
            if (chosen->waiting_time < 0) chosen->waiting_time = 0;

            completed++;
            gantt_record(state, chosen->pid, last_start, clock);
            current_pid[0] = '\0';
        }
    }

    // copy results back to state->processes (matching by PID)
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            if (strcmp(state->processes[j].pid, procs[i].pid) == 0) {
                state->processes[j].start_time   = procs[i].start_time;
                state->processes[j].finish_time   = procs[i].finish_time;
                state->processes[j].waiting_time  = procs[i].waiting_time;
                state->processes[j].started       = procs[i].started;
                state->processes[j].completed     = procs[i].completed;
                break;
            }

    free(procs);
    return 0;
}