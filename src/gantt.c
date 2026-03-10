#include <stdio.h>
#include <string.h>
#include "gantt.h"

void gantt_record(SchedulerState *state, const char *pid,
                  int start_time, int end_time) {
    if (state->gantt_size > 0) {
        GanttEntry *last = &state->gantt[state->gantt_size - 1];
        if (strcmp(last->pid, pid) == 0 && last->end_time == start_time) {
            last->end_time = end_time;   
            return;
        }
    }
    if (state->gantt_size >= MAX_GANTT_LEN) return;

    GanttEntry *e = &state->gantt[state->gantt_size++];
    strncpy(e->pid, pid, MAX_PID_LEN - 1);
    e->pid[MAX_PID_LEN - 1] = '\0';
    e->start_time = start_time;
    e->end_time   = end_time;
}

 // prints two-line ascii chart:
 // line 1 - process blocks [AAA][BBB]...
 // line 2 - time axis 0 240 420...
 // auto adjusts in long simulations 
void gantt_print(const SchedulerState *state) {
    if (state->gantt_size == 0) {
        printf("(empty Gantt chart)\n");
        return;
    }

    int total = state->gantt[state->gantt_size - 1].end_time;

    int scale = 1;
    while (total / scale > 72) scale++;

    printf("\n=== Gantt Chart ===\n");

   // process blocks line
    for (int i = 0; i < state->gantt_size; i++) {
        const GanttEntry *e = &state->gantt[i];
        int width = (e->end_time - e->start_time) / scale;
        if (width < 1) width = 1;

        printf("[");
        // center PID in block, padded with dashes
        int pid_len = (int)strlen(e->pid);
        int dashes  = width - pid_len;
        int left    = dashes / 2;
        int right   = dashes - left;
        for (int d = 0; d < left;  d++) putchar('-');
        printf("%s", e->pid);
        for (int d = 0; d < right; d++) putchar('-');
        printf("]");
    }
    printf("\n");

    //time axis line
    int printed_pos = 0;   

    for (int i = 0; i < state->gantt_size; i++) {
        const GanttEntry *e = &state->gantt[i];
        int width = (e->end_time - e->start_time) / scale;
        if (width < 1) width = 1;

       // print time label of the block
        char label[16];
        snprintf(label, sizeof(label), "%d", e->start_time);
        int label_len = (int)strlen(label);

        printf("%s", label);
        printed_pos += label_len;

        int next_col = printed_pos + (width + 2 - label_len); // +2 for [ ] 
    
        int block_end_col = printed_pos - label_len + width + 2;
        int pad = block_end_col - printed_pos;
        if (pad < 1) pad = 1;
        for (int s = 0; s < pad; s++) putchar(' ');
        printed_pos += pad;
        (void)next_col;
    }

    // print final time
    printf("%d\n", total);
}