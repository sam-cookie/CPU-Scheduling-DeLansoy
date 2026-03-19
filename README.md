
# CPU Scheduler Simulator
A modular CPU Scheduler Simulator designed to demonstrate and compare different CPU scheduling algorithms. The program processes workloads, calculates performance metrics, and generates an ASCII Gantt chart for visualization. This simulator also allows comparison between algorithms to better understand them.

Members: Del Rosario, Nina and Lansoy, Sam

## Features

**Multiple Scheduling Algorithms:**
- FCFS: First-Come, First-Served (Non-preemptive).
- SJF: Shortest Job First (Non-preemptive).
- STCF: Shortest Time-to-Completion First (Preemptive SJF).
- RR: Round Robin with configurable time quanta.
- MLFQ: Multi-Level Feedback Queue.
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
|-- utils.c: Shared utilities (Ready Queue management, comparison logic).
```

## Compilation and Usage

**Build:**
```
make
```
**Usage:**
- Use a text file with lines formatted as [PID] [Arrival_Time] [Burst_Time].

**Example:**
How to Run SJF with a workload file
```
./schedsim --algorithm=SJF --input=workload1.txt
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
