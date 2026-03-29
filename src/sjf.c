#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "scheduler.h"
#include "gantt.h"

// Shortest Job First (SJF)
// Implement non-preemptive SJF:

// Select process with shortest burst time from ready queue
// Once selected, runs to completion
// Optimal for average turnaround time (non-preemptive case)
// Requires sorting or priority queue (which is better?)

static int sjf_cmp(const Process *a, const Process *b) {
    if (a->burst_time != b->burst_time)
        return a->burst_time - b->burst_time;
    if (a->arrival_time != b->arrival_time)
        return a->arrival_time - b->arrival_time;
    return strcmp(a->pid, b->pid);
}

int schedule_sjf(SchedulerState *state) {
    int n = state->num_processes;

    // work on local copy
    Process *procs = malloc((size_t)n * sizeof(Process));
    if (!procs) return -1;
    memcpy(procs, state->processes, (size_t)n * sizeof(Process));

    int clock = 0, completed = 0;

    while (completed < n) {
        // among all that arrived, pick the one with shortest burst time
        Process *chosen = NULL;
        for (int i = 0; i < n; i++) {
            if (procs[i].completed || procs[i].arrival_time > clock) continue;
            if (chosen == NULL || sjf_cmp(&procs[i], chosen) < 0)
                chosen = &procs[i];
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

        // record start time (for response time)
        chosen->start_time = clock;
        chosen->started    = 1;

        int run_start = clock;
        clock += chosen->remaining_time;   // runs to completion

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

    // copy results back to caller's array
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            if (strcmp(state->processes[j].pid, procs[i].pid) == 0) {
                state->processes[j].start_time  = procs[i].start_time;
                state->processes[j].finish_time  = procs[i].finish_time;
                state->processes[j].waiting_time = procs[i].waiting_time; 
                state->processes[j].started      = procs[i].started;
                state->processes[j].completed    = procs[i].completed;
                break;
            }

    free(procs);
    return 0;
}