#ifndef GANTT_H
#define GANTT_H

#include "process.h"

// append slice to gantt log 
void gantt_record(SchedulerState *state, const char *pid,
                  int start_time, int end_time);

void gantt_print(const SchedulerState *state);

#endif /* GANTT_H */