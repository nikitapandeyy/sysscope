# Mini Process Monitor (`minimon`)

Mini Process Monitor is a clean, focused, and educational Linux process inspection tool written in modern C++17. It directly reads the Linux kernel's virtual `/proc` filesystem to track process execution, memory consumption, CPU utilization, and system-wide state without external libraries.

Every component in this repository is designed to be clear, readable, and explainable line-by-line.

## Planned Features

1. **Process Telemetry**: Read PID, PPID, process name, execution state, owner username, thread count, RSS/VmSize memory, and command lines.
2. **Accurate CPU%**: Calculated across two consecutive samples via `utime + stime` deltas and system clock ticks (`_SC_CLK_TCK`).
3. **Filtering & Searching**: Filter by process name substring, user, state (R, S, Z, etc.), PID, or minimum CPU% / memory thresholds.
4. **Sorting**: Sort process tables by CPU, memory, PID, or name in ascending or descending order.
5. **Interactive & One-shot Display**: Live auto-refreshing terminal table and single-shot reporting (`--once`).
6. **Edge Case Handling**: Handle processes dying during inspection (`ENOENT`), permission boundaries (`EACCES`), process names with spaces/parentheses, and kernel threads with empty command lines.
7. **JSON Export**: Export full process telemetry as structured JSON (`--json`).

## Architecture & Source Layout

- `src/proc_reader.*`: Raw `/proc` parsing supporting a configurable root path (e.g. for testing against fake fixtures).
- `src/process.*`: Core `Process` struct holding process telemetry.
- `src/cpu_tracker.*`: Stateful delta calculation for CPU% and PID reuse detection.
- `src/filter.*`: In-memory filtering and sorting algorithms.
- `src/display.*`: Terminal table formatting and live refresh loop.
- `src/main.cpp`: CLI argument handling and execution coordination.
- `tests/`: Automated unit and integration tests using GoogleTest.
- `experiments/`: Standalone exploratory programs investigating Linux system interfaces.

## Building and Running

```bash
# Configure build
cmake -B build -S .

# Compile
cmake --build build

# Run executable
./build/minimon
```
