#include <stdio.h>
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
    int      n     = state->num_processes;
    Process *procs = state->processes;

    /* initialise remaining_time for every process */
    for (int i = 0; i < n; i++) {
        procs[i].remaining_time = procs[i].burst_time;
        procs[i].completed      = 0;
        procs[i].started        = 0;
        procs[i].start_time     = -1;
    }

    int  clock      = 0;
    int  completed  = 0;
    int  last_start = 0;
    char current_pid[MAX_PID_LEN] = "";   /* pid of whoever is running */

    while (completed < n) {

        // pick process with shortest remaining_time
        Process *chosen = NULL;
        for (int i = 0; i < n; i++) {
            if (procs[i].completed || procs[i].arrival_time > clock) continue;
            if (chosen == NULL ||
                procs[i].remaining_time < chosen->remaining_time)
                chosen = &procs[i];
        }

        // CPU idle — no process ready yet
        if (chosen == NULL) {
            clock++;
            continue;
        }

        // context switch detection
        if (strcmp(current_pid, chosen->pid) != 0) {
            /* flush the previous process's slice to the Gantt chart */
            if (strlen(current_pid) > 0) {
                gantt_record(state, current_pid, last_start, clock);
                state->context_switches++;
            }
            /* record first execution for response time */
            if (!chosen->started) {
                chosen->start_time = clock;
                chosen->started    = 1;
            }
            strcpy(current_pid, chosen->pid);
            last_start = clock;
        }

        // run for 1 tick
        chosen->remaining_time--;
        clock++;

        // process finished
        if (chosen->remaining_time == 0) {
            chosen->finish_time = clock;
            chosen->completed   = 1;
            completed++;

            /* flush final slice */
            gantt_record(state, chosen->pid, last_start, clock);
            current_pid[0] = '\0';
        }
    }

    return 0;
}