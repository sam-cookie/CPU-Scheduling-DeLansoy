#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "scheduler.h"
#include "gantt.h"

// First-Come First-Serve (FCFS)
// Implement non-preemptive FCFS scheduling:

// Processes execute in order of arrival
// Once a process starts, it runs to completion
// Simple queue-based implementation

// compare two processes by arrival time, then PID for tie-breaking
static int arrival_cmp(const void *a, const void *b) {
    const Process *pa = (const Process *)a;
    const Process *pb = (const Process *)b;
    if (pa->arrival_time != pb->arrival_time)
        return pa->arrival_time - pb->arrival_time;
    return strcmp(pa->pid, pb->pid);
}

int schedule_fcfs(SchedulerState *state) {
    int n = state->num_processes;

    // work on local copy
    Process *procs = malloc((size_t)n * sizeof(Process));
    if (!procs) return -1;
    memcpy(procs, state->processes, (size_t)n * sizeof(Process));

    // sort by arrival time (and PID for tie-breaking)
    qsort(procs, (size_t)n, sizeof(Process), arrival_cmp);

    int clock = 0;
    int completed = 0;

    while (completed < n) {
        // find next process that has arrived and not completed
        Process *chosen = NULL;
        for (int i = 0; i < n; i++) {
            if (!procs[i].completed && procs[i].arrival_time <= clock) {
                chosen = &procs[i];
                break;
            }
        }

        if (chosen == NULL) {
            // cpu idle, advance to next arrival
            int next = INT_MAX;
            for (int i = 0; i < n; i++)
                if (!procs[i].completed && procs[i].arrival_time < next)
                    next = procs[i].arrival_time;
            gantt_record(state, "IDLE", clock, next);
            clock = next;
            continue;
        }

        // record start time and mark as started
        chosen->start_time = clock;
        chosen->started    = 1;

        int run_start = clock;
        clock += chosen->remaining_time;   // run til the end

        chosen->remaining_time = 0;
        chosen->finish_time    = clock;
        chosen->completed      = 1;

        // compute waiting_time in the local copy before copying back
        // so we no zero value into state->processes
        chosen->waiting_time =
            (chosen->finish_time - chosen->arrival_time) - chosen->burst_time;

        completed++;

        gantt_record(state, chosen->pid, run_start, clock);
    }

    // copy results back to state->processes (matching by PID)
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            if (strcmp(state->processes[j].pid, procs[i].pid) == 0) {
                state->processes[j].start_time  = procs[i].start_time;
                state->processes[j].finish_time  = procs[i].finish_time;
                state->processes[j].waiting_time = procs[i].waiting_time; // now valid
                state->processes[j].started      = procs[i].started;
                state->processes[j].completed    = procs[i].completed;
                break;
            }

    free(procs);
    return 0;
}