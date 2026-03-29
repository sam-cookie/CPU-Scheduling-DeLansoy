
# CPU Scheduler Simulator
A modular CPU Scheduler Simulator designed to demonstrate and compare different CPU scheduling algorithms. The program processes workloads, calculates performance metrics, and generates an ASCII Gantt chart for visualization. This simulator also allows comparison between algorithms to better understand them.

Members: Del Rosario, Nina and Lansoy, Sam

## Features

**Multiple Scheduling Algorithms:**
- FCFS: First-Come, First-Served (Non-preemptive).
    - Runs processes in arrival order, never interrupting a running process until it finishes.
- SJF: Shortest Job First (Non-preemptive).
    - Picks the process with the shortest burst time next, without preemption.
- STCF: Shortest Time-to-Completion First (Preemptive SJF).
    - Always runs whichever process has the least remaining time, preempting the current process if a shorter one arrives.
- RR: Round Robin with configurable time quantum.
    - Cycles through processes in order, giving each a fixed time quantum before switching to the next.
- MLFQ: Multi-Level Feedback Queue.
    - Uses multiple priority queues with different time quantums, automatically demoting CPU-heavy processes and periodically boosting all processes to prevent starvation.
- Visualization: Auto-scaling ASCII Gantt chart to visualize process execution and idle time.
- Performance Metrics: Detailed calculation of:
   * Turnaround Time (TT): Time from arrival to completion.
   * Waiting Time (WT): Total time spent in the ready queue. 
   * Response Time (RT): Time from arrival to first execution.

## Project Structure
```
|-- main.c: Entry point and command-line argument parsing.
|-- scheduler.h / process.h: Core data structures and scheduler interfaces.
|-- fcfs.c, sjf.c, stcf.c, rr.c: Logic for specific scheduling algorithms.
|-- mlfq.c: Multi-Level Feedback Queue implementation.
|-- metrics.c / metrics.h: Logic for calculating TT, WT, RT, and averages.
|-- gantt.c / gantt.h: ASCII Gantt chart generation logic.
|-- *_utils.c / *_utils.h: Modular utilities (ready queue, index queue, MLFQ state/config, output, comparison).
```

## Compilation and Usage

**Build:**
```
make
```
**Usage:**

Create a text file with lines formatted below:
```
// [PID] [Arrival_Time] [Burst_Time]
A 0  240
B 10 180
C 20 150
D 25  80
E 30 130
```

**Example:**
How to Run SJF with a text file
```
./schedsim --algorithm=SJF --input=text.txt
```

## Sample Output

```
=== Gantt Chart ===
[----------A----------][---D---][-----E-----][------C------][-------B--------]
0                      240      320          450            600             780

=== Metrics ===
Process  | AT   | BT   | FT     | TT     | WT     | RT    
---------|------|------|--------|--------|--------|--------
A        |    0 |  240 |    240 |    240 |      0 |      0
B        |   10 |  180 |    780 |    770 |    590 |    590
C        |   20 |  150 |    600 |    580 |    430 |    430
D        |   25 |   80 |    320 |    295 |    215 |    215
E        |   30 |  130 |    450 |    420 |    290 |    290
---------|------|------|--------|--------|--------|--------
Average  |      |      |        |  461.0 |  305.0 |  305.0
```
## Limitations and Assumptions
- Comparative analysis is not yet implemented.