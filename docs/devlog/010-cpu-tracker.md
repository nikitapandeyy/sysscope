# Devlog T2.7: Two-Sample CPU Tracker & PID Reuse Detection

## Task
Implement the stateful `CpuTracker` in `src/cpu_tracker.cpp` to compute per-process CPU utilization percentage using two-sample deltas across wall-clock time and clock ticks, handling edge cases such as new processes, non-advancing counters, PID reuse, and exited process pruning.

## What I did
- Authored `src/cpu_tracker.hpp` declaring `struct ProcessCpuSample` and class `CpuTracker`.
- Implemented `CpuTracker::calculate_cpu_percent()`:
  - Takes `prev_ticks`, `curr_ticks`, `prev_starttime`, `curr_starttime`, and `elapsed_seconds`.
  - Converts tick delta to CPU seconds using system clock ticks (`_SC_CLK_TCK`).
  - Divides CPU seconds by real wall-clock elapsed time:
    $$\text{CPU \%} = \frac{(\Delta \text{ticks} / \text{clk\_tck})}{\Delta t_{\text{seconds}}} \times 100.0$$
  - Handles edge cases: returns 0.0% if elapsed time $\le 0$, if counter did not advance ($\Delta \text{ticks} \le 0$), or if PID reuse is detected (`prev_starttime != curr_starttime`).
- Implemented `CpuTracker::update_process_cpu()`:
  - Maintains an internal history map (`std::unordered_map<pid_t, ProcessCpuSample>`).
  - Sets `cpu_percent = 0.0` upon the initial observation of any process.
  - Automatically updates the cached sample for subsequent cycles.
- Implemented `CpuTracker::prune_exited()`:
  - Removes stale PIDs that are no longer active, preventing memory leaks in long-running monitor loops.
- Added 7 unit tests in `tests/test_cpu_tracker.cpp` covering standard calculations, idle counters, backward counters, zero elapsed time, PID reuse detection, stateful lifecycle, and pruning.

## Why I did it this way
- CPU time counters in `/proc/[pid]/stat` (`utime` and `stime`) represent cumulative CPU time in clock ticks since process creation. They can only be converted to a utilization percentage by measuring the difference between two discrete time points.
- Detecting PID reuse via `starttime` (field 22 of `/proc/[pid]/stat`) is vital. Linux kernel PIDs wrap around (traditionally at 32768, or `cat /proc/sys/kernel/pid_max`). If a short-lived process terminates and another starts with the same PID, comparing the old ticks to the new ticks would produce wildly inaccurate CPU spikes. Checking that `starttime` matches ensures the sample belongs to the exact same execution instance.
- Alternatives considered: Using `/proc/stat` total CPU delta instead of wall-clock time. While valid, wall-clock delta calculation (standard top/Solaris mode) is less susceptible to multi-core skew and works seamlessly on individual process tracking.

## Problems encountered
None. The arithmetic and stateful tracking compiled cleanly with zero compiler warnings and passed all 29 tests.

## How I solved them
N/A (no problems encountered).

## Linux concepts involved
- Clock ticks (`sysconf(_SC_CLK_TCK)`): typical kernel user frequency `USER_HZ` (100 Hz).
- Monotonic time tracking: using `std::chrono::steady_clock` to measure elapsed wall-clock intervals immune to system clock adjustments (NTP shifts).
- PID reuse: kernel PID recycling and using `starttime` (jiffies since boot) as an immutable process fingerprint.

## How to verify
```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

## Open questions
- None. Next task is T2.8: unified process listing function that combines reading, CPU tracking, and memory accounting with graceful handling of vanishing processes.
