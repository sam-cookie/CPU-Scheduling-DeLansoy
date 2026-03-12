#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "process.h"
#include "scheduler.h"
#include "metrics.h"
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
 *      The queue runs in isolation during each quantum.
 *   5. If the CPU is idle, advance time to the next arrival.
 */
int schedule_rr(SchedulerState *state, int quantum) {
    printf("Running RR Scheduler (quantum = %d)...\n\n", quantum);

    int n          = state->num_processes;
    Process *procs = state->processes;

    // sort processes by arrival time 
    for (int i = 1; i < n; i++) {
        Process tmp = procs[i];
        int j = i - 1;
        while (j >= 0 && procs[j].arrival_time > tmp.arrival_time) {
            procs[j + 1] = procs[j];
            j--;
        }
        procs[j + 1] = tmp;
    }

    // ready queue 
    ReadyQueue *rq     = create_ready_queue(n * 2);
    int completed      = 0;
    int t              = 0;
    int next_to_arrive = 0;

    // enqueue all processes whose arrival_time <= `time`
    #define ENQUEUE_ARRIVALS(time) \
        while (next_to_arrive < n && \
               procs[next_to_arrive].arrival_time <= (time)) { \
            enqueue(rq, &procs[next_to_arrive]); \
            next_to_arrive++; \
        }

    // ── Main simulation loop ───────────────────────────────────── 
    while (completed < n) {

        ENQUEUE_ARRIVALS(t);

        // CPU idle — jump to next arrival 
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
            completed++;
        } else {
            enqueue(rq, current);  // ← preempted process goes back FIRST
            ENQUEUE_ARRIVALS(t); 
            state->context_switches++;
        }
    }

    #undef ENQUEUE_ARRIVALS

    // metrics & output (move outside)
    Metrics m;
    calculate_metrics(procs, n, &m);
    m.context_switches = state->context_switches;

    gantt_print(state);
    print_metrics(procs, n, &m);
    printf("\nTotal context switches: %d\n", state->context_switches);

    free_ready_queue(rq);
    return 0;
}