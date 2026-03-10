#!/bin/bash

# test_suite.sh  –  Automated tests for FCFS and SJF 

BIN=./schedsim
PASS=0
FAIL=0

# color helpers for pass/fail messages
GREEN='\033[0;32m'; RED='\033[0;31m'; RESET='\033[0m'
pass() { echo -e "  ${GREEN}PASS${RESET}  $1"; ((PASS++)); }
fail() { echo -e "  ${RED}FAIL${RESET}  $1"; ((FAIL++)); }

# helper: run schedsim, capture stdout, check for a pattern
check() {
    local desc="$1"; shift
    local pattern="$1"; shift
    local output
    output=$("$@" 2>&1)
    if echo "$output" | grep -qE "$pattern"; then
        pass "$desc"
    else
        fail "$desc  (expected pattern: '$pattern')"
        echo "    --- actual output (last 5 lines) ---"
        echo "$output" | tail -5 | sed 's/^/    /'
    fi
}

echo "============================================="
echo "      schedsim Test Suite  –  FCFS & SJF "
echo "============================================="
echo

# BUILD
echo "[BUILD]"
if make -s 2>&1 | grep -q "error:"; then
    echo -e "  ${RED}BUILD FAILED${RESET} – aborting tests."
    exit 1
else
    pass "make succeeds without errors"
fi
echo

echo "[BINARY]"
check "binary exists"          "Usage:"  $BIN --help
check "exits 0 on --help"      "Usage:"  $BIN --help
echo

# workload1.txt tests (from lecture quiz)
echo "[FCFS – workload1.txt  (lecture quiz values)]"

# avg TT should be ~515 
check "FCFS avg TT ≈ 515" \
    "515|514|516" \
    $BIN --algorithm=FCFS --input=tests/workload1.txt

# process A should finish at t=240 (it arrives at 0, burst=240)
check "FCFS: process A finishes at 240" \
    "240" \
    $BIN --algorithm=FCFS --input=tests/workload1.txt

# ganttchart should show all processes
# PIDs are centred inside blocks: [---A---] so match the PID directly
for pid in A B C D E; do
    check "FCFS: process $pid appears in Gantt" \
        "$pid" \
        $BIN --algorithm=FCFS --input=tests/workload1.txt
done
echo

# FCFS inline processes
echo "[FCFS – inline --processes flag]"
check "FCFS inline: runs without error" \
    "Gantt" \
    $BIN --algorithm=FCFS --processes="A:0:240,B:10:180,C:20:150"
echo

#FCFS edge cases
echo "[FCFS – edge cases]"

# single process
check "FCFS single process: exits 0" \
    "Gantt" \
    $BIN --algorithm=FCFS --processes="X:0:100"

# simultaneous arrivals (workload2.txt) 
check "FCFS simultaneous arrivals: runs without error" \
    "Gantt" \
    $BIN --algorithm=FCFS --input=tests/workload2.txt

# all same burst time – SJF should behave like FCFS
check "FCFS identical burst times: all processes appear" \
    "P1.*P2.*P3|P3.*P2.*P1" \
    $BIN --algorithm=FCFS --input=tests/workload2.txt
echo

#sjf workload 
echo "[SJF – workload1.txt  (lecture quiz values)]"

# avg TT should be ~461 
check "SJF avg TT ≈ 461" \
    "461|460|462" \
    $BIN --algorithm=SJF --input=tests/workload1.txt

# SJF should schedule D (burst=80) before B (burst=180) in the Gantt line
output=$($BIN --algorithm=SJF --input=tests/workload1.txt 2>&1)
gantt_line=$(echo "$output" | grep "^\[")
pos_D=$(echo "$gantt_line" | grep -bo "D" | head -1 | cut -d: -f1)
pos_B=$(echo "$gantt_line" | grep -bo "B" | head -1 | cut -d: -f1)
if [ -n "$pos_D" ] && [ -n "$pos_B" ] && [ "$pos_D" -lt "$pos_B" ]; then
    pass "SJF: shortest job (D, burst=80) scheduled before longer job (B, burst=180)"
else
    fail "SJF: D should appear before B in Gantt (D pos=$pos_D, B pos=$pos_B)"
fi

# All processes finish
for pid in A B C D E; do
    check "SJF: process $pid appears in output" \
        "$pid" \
        $BIN --algorithm=SJF --input=tests/workload1.txt
done
echo

echo "[SJF – edge cases]"

check "SJF single process: exits cleanly" \
    "Gantt" \
    $BIN --algorithm=SJF --processes="Only:0:50"

check "SJF simultaneous arrivals: picks shortest first" \
    "Gantt" \
    $BIN --algorithm=SJF --input=tests/workload2.txt

# if same burst time, sjf runs like fcfs
check "SJF identical burst times: all three run" \
    "P1.*P2.*P3|P2.*P1.*P3|P3.*P1.*P2" \
    $BIN --algorithm=SJF --input=tests/workload2.txt
echo

# metrics sanity check hehe
echo "[Metrics sanity]"

# if p arrive at 0 with burst 100:
#   FT=100, TT=100, WT=0, RT=0
output=$($BIN --algorithm=FCFS --processes="Z:0:100" 2>&1)
if echo "$output" | grep -qE "100.*100.*0.*0|Z.*100"; then
    pass "Single process: correct TT/WT/RT values"
else
    fail "Single process: unexpected metrics"
    echo "$output" | tail -10 | sed 's/^/    /'
fi

# if p arrive at 50 with burst 30, scheduled at t=50:
#   FT=80, TT=30, WT=0, RT=0
output=$($BIN --algorithm=FCFS --processes="W:50:30" 2>&1)
if echo "$output" | grep -qE "80|30.*0.*0"; then
    pass "Late-arriving single process: correct finish time"
else
    fail "Late-arriving single process: unexpected values"
    echo "$output" | tail -10 | sed 's/^/    /'
fi
echo

# error handling
echo "[Error handling]"

# missing input should exit non-zero
$BIN --algorithm=FCFS 2>/dev/null
[ $? -ne 0 ] && pass "No input: exits non-zero" || fail "No input: should exit non-zero"

# non-existent file should exit non-zero
$BIN --algorithm=FCFS --input=tests/doesnotexist.txt 2>/dev/null
[ $? -ne 0 ] && pass "Missing file: exits non-zero" || fail "Missing file: should exit non-zero"


check "Unknown algorithm: falls back to FCFS" \
    "FCFS|Gantt" \
    $BIN --algorithm=BOGUS --processes="A:0:10"
echo

# summary
echo "============================================="
echo " Results:  ${PASS} passed,  ${FAIL} failed"
echo "============================================="
[ $FAIL -eq 0 ] && exit 0 || exit 1