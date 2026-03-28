# Multi-Level Feedback Queue
Uses multiple priority queues with different time quantums, automatically demoting CPU-heavy processes and periodically boosting all processes to prevent starvation

## Design Justification:
We implemented a three-level MLFQ with a high-priority Q0 (quantum=10ms, allotment=50ms), a medium-priority Q1 (quantum=30ms, allotment=150ms), and a lowest-priority FCFS Q2 to effectively balance responsiveness and throughput without prior knowledge of process burst times. Three levels were chosen because our workloads naturally fell into three categories: short-to-medium jobs that cycle through Q0, longer jobs that graduate to Q1, and any remaining long-running batch jobs handled by FCFS in Q2, and the simulation (sample output below) confirmed all five processes completed without ever reaching Q2. This configuration is justified by the simulation trace (sample output below), which shows an exceptional Average Response Time of 13.0ms, as the small 10ms quantum in Q0 allowed new arrivals (like Process B and C) to gain CPU access almost immediately. The 50ms allotment prevents any single process from monopolizing the high-priority tier; for instance, Process A was correctly demoted to Q1 at t=180 after exhausting its Q0 budget, allowing other jobs to progress. Q1's 150ms allotment (5 × 30ms quantum) gives medium-priority jobs sufficient runway where Process C ran from t=350 to t=380 and Process D from t=380 to t=410 in Q1 before D completed at t=410 with a turnaround of 385ms, confirming the allotment is large enough to allow meaningful progress. Additionally, the 200ms Priority Boost was important to prevent starvation for long-running tasks, just like what we see at t=200 and t=400 when all processes were reset to Q0; this ensures that even heavy jobs like Process A (240 total CPU ticks) eventually finish alongside shorter ones, as confirmed by its completion at t=780 with a turnaround of 780ms. Overall, the simulation results are validated with our design resulting the workload's Average Turnaround Time of 643.0ms and Average Waiting Time of 487.0ms, where the shortest job (D, 80 ticks) completed first and the longest (A, 240 ticks) finished last.

## Comparison with Standard 3-Level MLFQ:
A standard 3-level MLFQ typically uses equal or uniformly increasing time quantums across queues with no explicit allotment cap in which a process stays in a queue until it uses its full quantum repeatedly. On the other hand, our implementation differs on how we use an explicit allotment per queue (50ms for Q0, 150ms for Q1) rather than relying solely on per-slice preemption, and how we include a configurable priority boost period of 200ms. The allotment-based demotion identifies CPU-heavy processes and moves them down, while the boost period is a way to prevent starvation.

## Tradeoffs
The primary tradeoff in our design is the responsiveness vs turnaround time for CPU-heavy jobs. The 10ms quantum in Q0 helps keep response time low (13.0ms average) but it results frequent context switches. In Phase 1 alone, the CPU switched processes every 10ms across five jobs. This is acceptable for interactive workloads but adds overhead for long-running jobs just like with Process A finishing at t=780 despite arriving first. Additionally, Q2 was never reached in our test workload, meaning the FCFS tier exists as a safety net rather than an active component which would make a mixed workload with truly short jobs alongside very long ones exercise it more. Finally, the 200ms boost period resets all allotments uniformly, which temporarily disrupts the natural priority ordering every boost cycle, slightly inflating turnaround time in exchange for starvation prevention.

## Implementation:

**Dynamic Preemption**
```
/* run the slice tick by tick so mid-slice arrivals
 * can enter Q0 without waiting until the next turn */
for (int tick = 0; tick < slice; tick++) {
    p->remaining_time--;
    mlfq->time_in_queue[idx]++;
    p->total_cpu_time++;
    t++;
    /* catch any arrivals that sneak in during this slice */
    while (next_to_arrive < n && procs[next_to_arrive].arrival_time <= t) {
        mlfq_enqueue_process(mlfq, procs, next_to_arrive, 0, t, slice_start, p->pid);
        next_to_arrive++;
    }
}
```
**Prevent Gaming Via Yielding**
```
/* still has work — demote if allotment burned, else re-enqueue */
int old_lvl = p->priority;
int used    = mlfq->time_in_queue[idx];

mlfq_requeue_or_demote(mlfq, idx, procs, config);

if (p->priority != old_lvl) {
    printf("t=%-3d: Process %s → Q%d (exhausted %dms Q%d allotment)\n",
           t, p->pid, p->priority, used, old_lvl);
}
```
**Starvation Prevention (Priority Boost)**
```
/* boost all processes to Q0 if enough time has passed */
if (mlfq->boost_period > 0) {
    if (mlfq_boost(mlfq, procs, t)) {
        phase++;
        printf("\n--- Phase %d: t=%d to t=%d (next boost period) ---\n\n",
               phase, t, t + mlfq->boost_period);
        mlfq_print_phase_summary(procs, n, t);
    }
}
```

## Example Output: 
```
=== Analysis ===
  A: long job  — used 240 total cpu ticks
  B: long job  — used 180 total cpu ticks
  C: long job  — used 150 total cpu ticks
  D: long job  — used 80 total cpu ticks
  E: long job  — used 130 total cpu ticks

=== Gantt Chart ===
[A][B][A][C][B][D][E][A][C][B][D][E][A][C][B][D][E][A][C][B][D][E][C][A][B][D][E][A][B][A][B][A][B][A][B][C-][D-][E][A][B][C][E][A][B][C][E][A][B][C][E][A][B][C][E][A][B][C][E][A][B][C][E][A][B][C][E][A][B][--A--]
0  10 20 30 40 50 60 70 80 90 100 110 120 130 140 150 160 170 180 190 200 210 220 230 240 250 260 270 280 290 300 310 320 330 340 350 380 410 420 430 440 450 460 470 480 490 500 510 520 530 540 550 560 570 580 590 600 610 620 630 640 650 660 670 680 690 700 710 720    780

=== Metrics ===
Process  | AT   | BT   | FT     | TT     | WT     | RT    
---------|------|------|--------|--------|--------|--------
A        |    0 |  240 |    780 |    780 |    540 |      0
B        |   10 |  180 |    720 |    710 |    530 |      0
C        |   20 |  150 |    690 |    670 |    520 |     10
D        |   25 |   80 |    410 |    385 |    305 |     25
E        |   30 |  130 |    700 |    670 |    540 |     30
---------|------|------|--------|--------|--------|--------
Average  |      |      |        |  643.0 |  487.0 |   13.0
```