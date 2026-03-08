#ifndef PROCESS_H
#define PROCESS_H

#define MAX_PID_LEN 16
#define MAX_PROCESSES 256
#define MAX_GANTT_LEN 100000

//main structs
typedef struct {
    char pid[MAX_PID_LEN]; 
    int arrival_time;      
    int burst_time;         
    int remaining_time;     //for pre-emptive schedulers

    int start_time;        
    int finish_time;        
    int waiting_time;      

    //mlfq 
    int priority;           // current queue level (0 = highest)          
    int time_in_queue;      // accumulated time in current queue level     
    int total_cpu_time;     // total CPU time consumed so far              

    //state flags
    int started;            // 1 if process has been executed at least once 
    int completed;          // 1 if process has finished                    
} Process;


//gannt chart entry 
typedef struct {
    char pid[MAX_PID_LEN];  /* Which process ran (or "IDLE")  */
    int start_time;         /* Start of this slice            */
    int end_time;           /* End of this slice              */
} GanttEntry;


//simulation
typedef struct {
    Process *processes;         /* Array of all processes           */
    int num_processes;          /* Number of processes              */
    int current_time;           /* Current simulation clock         */
    int context_switches;       /* Total context switches           */

    GanttEntry gantt[MAX_GANTT_LEN]; /* Gantt chart log             */
    int gantt_size;                  /* Number of entries logged    */
} SchedulerState;


Process *create_processes(int n);
void free_processes(Process *processes);

#endif