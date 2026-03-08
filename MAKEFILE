CC      = gcc
CFLAGS  = -Wall -Wextra -Wpedantic -g -Iinclude
TARGET  = schedsim

SRC = src/main.c     \
      src/process.c  \
      src/fcfs.c     \
      src/sjf.c      \
      src/stcf.c     \
      src/rr.c       \
      src/mlfq.c     \
      src/metrics.c  \
      src/gantt.c    \
      src/utils.c

OBJ = $(SRC:.c=.o)

# ── Build ────────────────────────────────────────────────────────────
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# ── Convenience run targets ──────────────────────────────────────────
run-fcfs: $(TARGET)
	./$(TARGET) --algorithm=FCFS --input=tests/workload1.txt

run-sjf: $(TARGET)
	./$(TARGET) --algorithm=SJF --input=tests/workload1.txt

run-stcf: $(TARGET)
	./$(TARGET) --algorithm=STCF --input=tests/workload1.txt

run-rr: $(TARGET)
	./$(TARGET) --algorithm=RR --quantum=30 --input=tests/workload1.txt

run-mlfq: $(TARGET)
	./$(TARGET) --algorithm=MLFQ --input=tests/workload1.txt

run-compare: $(TARGET)
	./$(TARGET) --compare --input=tests/workload1.txt

# ── Tests ────────────────────────────────────────────────────────────
test: $(TARGET)
	bash tests/test_suite.sh

# ── Cleanup ──────────────────────────────────────────────────────────
clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean test run-fcfs run-sjf run-stcf run-rr run-mlfq run-compare