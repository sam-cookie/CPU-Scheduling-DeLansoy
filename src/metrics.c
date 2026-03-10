#include <stdio.h>
#include <string.h>
#include "metrics.h"

void calculate_metrics(Process *processes, int n, Metrics *out) {
    double sum_tt = 0, sum_wt = 0, sum_rt = 0;

    for (int i = 0; i < n; i++) {
        Process *p = &processes[i];

        int tt = p->finish_time - p->arrival_time;   // turn around time = finish - arrival
        int wt = tt - p->burst_time;                  // waiting time = turn around - burst
        int rt = p->start_time - p->arrival_time;   // response time = start - arrival

        p->waiting_time = wt;

        sum_tt += tt;
        sum_wt += wt;
        sum_rt += rt;
    }

    out->avg_turnaround = sum_tt / n;
    out->avg_waiting    = sum_wt / n;
    out->avg_response   = sum_rt / n;
}

void print_metrics(const Process *processes, int n, const Metrics *m) {
    printf("\n=== Metrics ===\n");
    printf("%-8s | %-4s | %-4s | %-6s | %-6s | %-6s | %-6s\n",
           "Process", "AT", "BT", "FT", "TT", "WT", "RT");
    printf("---------|------|------|--------|--------|--------|--------\n");

    for (int i = 0; i < n; i++) {
        const Process *p = &processes[i];
        int tt = p->finish_time  - p->arrival_time;
        int wt = tt - p->burst_time;
        int rt = p->start_time   - p->arrival_time;

        printf("%-8s | %4d | %4d | %6d | %6d | %6d | %6d\n",
               p->pid, p->arrival_time, p->burst_time,
               p->finish_time, tt, wt, rt);
    }

    printf("---------|------|------|--------|--------|--------|--------\n");
    printf("%-8s |      |      |        | %6.1f | %6.1f | %6.1f\n",
           "Average", m->avg_turnaround, m->avg_waiting, m->avg_response);
}