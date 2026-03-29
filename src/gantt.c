#include <stdio.h>
#include <string.h>
#include "gantt.h"

void gantt_record(SchedulerState *state, const char *pid,
                  int start_time, int end_time) {
    if (end_time <= start_time) return;

    if (state->gantt_size > 0) {
        GanttEntry *last = &state->gantt[state->gantt_size - 1];
        if (strncmp(last->pid, pid, MAX_PID_LEN - 1) == 0
                && last->end_time == start_time) {
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

void gantt_print(const SchedulerState *state) {
    if (state->gantt_size == 0) {
        printf("(empty Gantt chart)\n");
        return;
    }

    int total = state->gantt[state->gantt_size - 1].end_time;
    if (total <= 0) {
        printf("(empty Gantt chart)\n");
        return;
    }

    // Find smallest scale where total screen width fits in MAX_COLS.
    int scale = 1;
    for (; scale <= total; scale++) {
        int screen_width = 0;
        for (int i = 0; i < state->gantt_size; i++) {
            int raw   = state->gantt[i].end_time - state->gantt[i].start_time;
            int width = raw / scale;
            if (width < MIN_BLOCK_WIDTH) width = MIN_BLOCK_WIDTH;
            screen_width += width + 2;   // '[' + content + ']'
        }
        if (screen_width <= MAX_COLS) break;
    }

    // precompute widths at chosen scale
    int widths[MAX_GANTT_LEN];
    for (int i = 0; i < state->gantt_size; i++) {
        int raw   = state->gantt[i].end_time - state->gantt[i].start_time;
        widths[i] = raw / scale;
        if (widths[i] < MIN_BLOCK_WIDTH) widths[i] = MIN_BLOCK_WIDTH;
    }

    printf("\n=== Gantt Chart ===\n");

    int row_start = 0;
    while (row_start < state->gantt_size) {

        // greedily pack blocks into this row
        int row_width = 0;
        int row_end   = row_start;
        while (row_end < state->gantt_size) {
            int needed = widths[row_end] + 2;
            if (row_width + needed > MAX_COLS && row_end > row_start) break;
            row_width += needed;
            row_end++;
        }

        // block line
        for (int i = row_start; i < row_end; i++) {
            const GanttEntry *e = &state->gantt[i];
            int width   = widths[i];
            int pid_len = (int)strlen(e->pid);

            printf("[");
            if (pid_len >= width) {
                for (int c = 0; c < width; c++) putchar(e->pid[c]);
            } else {
                int dashes = width - pid_len;
                int left   = dashes / 2;
                int right  = dashes - left;
                for (int d = 0; d < left;  d++) putchar('-');
                printf("%s", e->pid);
                for (int d = 0; d < right; d++) putchar('-');
            }
            printf("]");
        }
        printf("\n");

        // time axis — every block is now wide enough to show its label
        int cursor   = 0;
        int last_end = -1;
        int col      = 0;

        for (int i = row_start; i < row_end; i++) {
            char label[16];
            snprintf(label, sizeof(label), "%d", state->gantt[i].start_time);
            int label_len = (int)strlen(label);

            if (col >= last_end) {
                while (cursor < col) { putchar(' '); cursor++; }
                printf("%s", label);
                cursor   += label_len;
                last_end  = cursor;
            }
            col += widths[i] + 2;
        }

        // end-time of last block in this row
        {
            char label[16];
            snprintf(label, sizeof(label), "%d",
                     state->gantt[row_end - 1].end_time);
            int label_len = (int)strlen(label);
            if (col >= last_end) {
                while (cursor < col) { putchar(' '); cursor++; }
                printf("%s", label);
                (void)label_len;
            }
        }

        printf("\n");
        if (row_end < state->gantt_size) printf("\n");

        row_start = row_end;
    }
}