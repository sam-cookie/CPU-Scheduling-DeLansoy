#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scheduler.h"
#include "gantt.h"

/*
 * RR — Round Robin (preemptive, fixed time quantum)
 *
 * Rules:
 *   1. Processes are served in FIFO order from the ready queue.
 *   2. Each process runs for at most `quantum` time units per turn.
 *   3. If a process doesn't finish within its quantum, it goes to
 *      the back of the ready queue (context switch).
 *   4. NEW ARRIVALS during a running quantum join the queue at the
 *      end — they do NOT preempt the currently running process.
 *   5. If the CPU is idle, advance time to the next arrival.
 */
int schedule_rr(SchedulerState *state, int quantum) {
    if (quantum <= 0) {
        fprintf(stderr, "Error: RR quantum must be positive, got %d\n", quantum);
        return -1;
    }

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

    // sort by arrival time
    for (int i = 1; i < n; i++) {
        Process tmp = procs[i];
        int j = i - 1;
        while (j >= 0 && procs[j].arrival_time > tmp.arrival_time) {
            procs[j + 1] = procs[j];
            j--;
        }
        procs[j + 1] = tmp;
    }

    ReadyQueue *rq     = create_ready_queue(n * 2);
    int completed      = 0;
    int t              = 0;
    int next_to_arrive = 0;

    #define ENQUEUE_ARRIVALS(time) \
        while (next_to_arrive < n && \
               procs[next_to_arrive].arrival_time <= (time)) { \
            if (!enqueue(rq, &procs[next_to_arrive])) { \
                fprintf(stderr, "Error: failed to enqueue arriving process %s\n", \
                        procs[next_to_arrive].pid); \
            } \
            next_to_arrive++; \
        }

    while (completed < n) {

        ENQUEUE_ARRIVALS(t);

        if (queue_is_empty(rq)) {
            if (next_to_arrive < n) {
                t = procs[next_to_arrive].arrival_time;
                ENQUEUE_ARRIVALS(t);
            } else {
                break;
            }
        }

        Process *current = dequeue(rq);

        if (!current->started) {
            current->start_time = t;
            current->started    = 1;
        }

        int slice       = (current->remaining_time < quantum)
                          ? current->remaining_time : quantum;
        int slice_start = t;

        t += slice;
        current->remaining_time -= slice;

        gantt_record(state, current->pid, slice_start, t);

        if (current->remaining_time == 0) {
            current->finish_time = t;
            current->completed   = 1;

            // compute waiting time before copying back
            current->waiting_time =
                (current->finish_time - current->arrival_time) - current->burst_time;
            if (current->waiting_time < 0) current->waiting_time = 0;

            completed++;
        } else {
            ENQUEUE_ARRIVALS(t);
            if (!enqueue(rq, current)) {
                fprintf(stderr, "Error: failed to re-enqueue process %s\n",
                        current->pid);
            }
            state->context_switches++;
        }
    }

    #undef ENQUEUE_ARRIVALS

    free_ready_queue(rq);

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