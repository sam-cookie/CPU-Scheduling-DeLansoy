#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "process.h"
#include "scheduler.h"
#include "metrics.h"
#include "gantt.h"

//easier running
// $ ./schedsim --algorithm=FCFS --input=workload.txt

//forward declarations (put parser functions here since there is no separate parser file)

static void usage(const char *prog);
static Process *parse_input_file(const char *path, int *n);
static Process *parse_inline_processes(const char *spec, int *n);
static SchedulingAlgorithm parse_algorithm(const char *name);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

    SchedulingAlgorithm  algo      = ALGO_FCFS;
    const char          *algo_name = "FCFS";
    const char          *input_file = NULL;
    const char          *proc_deets = NULL;
    const char          *mlfq_cfg   = NULL;
    int                  rr_quantum = 30;

    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--algorithm=", 12) == 0) {
            algo_name = argv[i] + 12;
            algo      = parse_algorithm(algo_name);
        } else if (strncmp(argv[i], "--input=", 8) == 0) {
            input_file = argv[i] + 8;
        } else if (strncmp(argv[i], "--processes=", 12) == 0) {
            proc_deets = argv[i] + 12;
        } else if (strncmp(argv[i], "--quantum=", 10) == 0) {
            rr_quantum = atoi(argv[i] + 10);
            if (rr_quantum <= 0) {
                fprintf(stderr, "Error: RR quantum must be positive, got %d\n", rr_quantum);
                return 1;
            }
        } else if (strncmp(argv[i], "--mlfq-config=", 14) == 0) {
            mlfq_cfg = argv[i] + 14;
        } else if (strcmp(argv[i], "--compare") == 0) {
            algo      = ALGO_COMPARE;
            algo_name = "COMPARE";
        } else if (strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Unknown argument: %s\n", argv[i]);
            usage(argv[0]);
            return 1;
        }
    }

    int      num_processes = 0;
    Process *processes     = NULL;

    if (input_file) {
        processes = parse_input_file(input_file, &num_processes);
    } else if (proc_deets) {
        processes = parse_inline_processes(proc_deets, &num_processes);
    } else {
        fprintf(stderr, "Error: provide --input=<file> or --processes=<spec>\n");
        return 1;
    }

    if (!processes || num_processes == 0) {
        fprintf(stderr, "Error: no processes loaded.\n");
        return 1;
    }

    MLFQConfig *mlfq_config = NULL;
    if (algo == ALGO_MLFQ || algo == ALGO_COMPARE) {
        mlfq_config = mlfq_cfg ? load_mlfq_config(mlfq_cfg)
                                : default_mlfq_config();
        if (!mlfq_config) {
            fprintf(stderr, "Error: failed to load MLFQ config.\n");
            free(processes);
            return 1;
        }
    }

    int result = 0;

    if (algo == ALGO_COMPARE) {
        result = schedule_compare(processes, num_processes,
                                  rr_quantum, mlfq_config);
    } else {
        // allocate SchedulerState on the heap instead of the stack.
        SchedulerState *state = calloc(1, sizeof(SchedulerState));
        if (!state) {
            fprintf(stderr, "Error: failed to allocate scheduler state.\n");
            free(processes);
            free_mlfq_config(mlfq_config);
            return 1;
        }
        state->processes     = processes;
        state->num_processes = num_processes;

        switch (algo) {
            case ALGO_FCFS:  result = schedule_fcfs(state);              break;
            case ALGO_SJF:   result = schedule_sjf(state);               break;
            case ALGO_STCF:  result = schedule_stcf(state);              break;
            case ALGO_RR:    result = schedule_rr(state, rr_quantum);    break;
            case ALGO_MLFQ:  result = schedule_mlfq(state, mlfq_config); break;
            default:
                fprintf(stderr, "Unknown algorithm.\n");
                result = 1;
        }

        if (result == 0)
            print_results(state, algo_name);

        free(state->gantt);   // free the heap-allocated gantt array
        free(state);
    }

    free(processes);
    free_mlfq_config(mlfq_config);
    return result;
}

static void usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s [OPTIONS]\n"
        "\n"
        "Options:\n"
        "  --algorithm=<FCFS|SJF|STCF|RR|MLFQ>  Scheduling algorithm\n"
        "  --input=<file>                         Process workload file\n"
        "  --processes=<PID:AT:BT,...>            Inline process spec\n"
        "  --quantum=<n>                          RR time quantum (default: 30)\n"
        "  --mlfq-config=<file>                   MLFQ config file\n"
        "  --compare                              Run all algorithms\n"
        "  --help                                 Show this message\n"
        "\n"
        "Example:\n"
        "  %s --algorithm=FCFS --input=workload.txt\n"
        "  %s --algorithm=RR --quantum=50 --processes=\"A:0:240,B:10:180\"\n",
        prog, prog, prog);
}

static SchedulingAlgorithm parse_algorithm(const char *name) {
    if (strcmp(name, "FCFS") == 0) return ALGO_FCFS;
    if (strcmp(name, "SJF")  == 0) return ALGO_SJF;
    if (strcmp(name, "STCF") == 0) return ALGO_STCF;
    if (strcmp(name, "RR")   == 0) return ALGO_RR;
    if (strcmp(name, "MLFQ") == 0) return ALGO_MLFQ;
    fprintf(stderr, "Unknown algorithm '%s', defaulting to FCFS.\n", name);
    return ALGO_FCFS;
}

static Process *parse_input_file(const char *path, int *n) {
    FILE *f = fopen(path, "r");
    if (!f) { perror(path); return NULL; }

    Process *buf = malloc(MAX_PROCESSES * sizeof(Process));
    if (!buf) { fclose(f); return NULL; }

    *n = 0;
    char line[256];

    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;

        Process *p = &buf[*n];
        memset(p, 0, sizeof(Process));
        p->start_time = -1;

        if (sscanf(line, "%15s %d %d",
                   p->pid, &p->arrival_time, &p->burst_time) != 3) {
            fprintf(stderr, "Warning: skipping line with wrong or missing format: %s", line);
            continue;
        }

        p->remaining_time = p->burst_time;
        (*n)++;

        if (*n >= MAX_PROCESSES) {
            fprintf(stderr, "Warning: process limit (%d) reached.\n", MAX_PROCESSES);
            break;
        }
    }

    fclose(f);
    return buf;
}

static Process *parse_inline_processes(const char *spec, int *n) {
    Process *buf = malloc(MAX_PROCESSES * sizeof(Process));
    if (!buf) return NULL;

    *n = 0;
    char copy[4096];
    strncpy(copy, spec, sizeof(copy) - 1);
    copy[sizeof(copy) - 1] = '\0';

    char *token = strtok(copy, ",");
    while (token && *n < MAX_PROCESSES) {
        Process *p = &buf[*n];
        memset(p, 0, sizeof(Process));
        p->start_time = -1;

        if (sscanf(token, "%15[^:]:%d:%d",
                   p->pid, &p->arrival_time, &p->burst_time) != 3) {
            fprintf(stderr, "Warning: skipping line with wrong or missing format: %s\n", token);
            token = strtok(NULL, ",");
            continue;
        }

        p->remaining_time = p->burst_time;
        (*n)++;
        token = strtok(NULL, ",");
    }

    return buf;
}