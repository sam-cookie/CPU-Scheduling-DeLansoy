#ifndef METRICS_H
#define METRICS_H

#include "process.h"

typedef struct {
    double avg_turnaround;
    double avg_waiting;
    double avg_response;
    int    context_switches;
} Metrics;


//  turnaround = finish_time  - arrival_time
//  waiting    = turnaround   - burst_time
//  response   = start_time   - arrival_time
 

void     calculate_metrics(Process *processes, int n, Metrics *out);
void     print_metrics(const Process *processes, int n, const Metrics *m);

#endif /* METRICS_H */