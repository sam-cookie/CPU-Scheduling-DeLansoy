#include <stdio.h>
#include <string.h>
#include "metrics.h"

void calculate_metrics(Process *processes, int n, Metrics *out) {
    if (n <= 0) {
        out->avg_turnaround = 0;
        out->avg_waiting    = 0;
        out->avg_response   = 0;
        return;
    }

    double sum_tt = 0, sum_wt = 0, sum_rt = 0;
    int valid = 0;   // count only processes that pass validation

    for (int i = 0; i < n; i++) {
        Process *p = &processes[i];

        // validate per-process invariants: start_time and finish_time must
        // both be >= arrival_time, and finish_time >= start_time
        if (p->start_time  < p->arrival_time ||
            p->finish_time < p->arrival_time ||
            p->finish_time < p->start_time) {
            fprintf(stderr,
                "Warning: process %s has invalid times "
                "(AT=%d ST=%d FT=%d) — excluded from averages.\n",
                p->pid, p->arrival_time, p->start_time, p->finish_time);
            continue;
        }

        int tt = p->finish_time - p->arrival_time;  // turnaround
        int wt = tt - p->burst_time;                 // waiting
        int rt = p->start_time  - p->arrival_time;  // response

        if (wt < 0) wt = 0;

        p->waiting_time = wt;

        sum_tt += tt;
        sum_wt += wt;
        sum_rt += rt;
        valid++;
    }

    if (valid > 0) {
        out->avg_turnaround = sum_tt / valid;
        out->avg_waiting    = sum_wt / valid;
        out->avg_response   = sum_rt / valid;
    } else {
        out->avg_turnaround = 0;
        out->avg_waiting    = 0;
        out->avg_response   = 0;
    }
}

void print_metrics(const Process *processes, int n, const Metrics *m) {
    printf("\n=== Metrics ===\n");
    printf("%-8s | %-4s | %-4s | %-6s | %-6s | %-6s | %-6s\n",
           "Process", "AT", "BT", "FT", "TT", "WT", "RT");
    printf("---------|------|------|--------|--------|--------|--------\n");

    for (int i = 0; i < n; i++) {
        const Process *p = &processes[i];

        // validate same invariants as calculate_metrics before printing
        if (p->start_time  < p->arrival_time ||
            p->finish_time < p->arrival_time ||
            p->finish_time < p->start_time) {
            printf("%-8s | %4d | %4d | %6s | %6s | %6s | %6s\n",
                   p->pid, p->arrival_time, p->burst_time,
                   "?", "?", "?", "?");
            continue;
        }

        int tt = p->finish_time - p->arrival_time;
        int wt = tt - p->burst_time;
        int rt = p->start_time  - p->arrival_time;
        if (wt < 0) wt = 0;

        printf("%-8s | %4d | %4d | %6d | %6d | %6d | %6d\n",
               p->pid, p->arrival_time, p->burst_time,
               p->finish_time, tt, wt, rt);
    }

    printf("---------|------|------|--------|--------|--------|--------\n");
    printf("%-8s |      |      |        | %6.1f | %6.1f | %6.1f\n",
           "Average", m->avg_turnaround, m->avg_waiting, m->avg_response);
}