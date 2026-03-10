#include <stdlib.h>
#include <string.h>
#include "process.h"

Process *create_processes(int n) {
    Process *p = calloc(n, sizeof(Process));
    if (!p) return NULL;
    for (int i = 0; i < n; i++) {
        p[i].start_time = -1;   // -1 = not yet started 
    }
    return p;
}

void free_processes(Process *processes) {
    free(processes);
}