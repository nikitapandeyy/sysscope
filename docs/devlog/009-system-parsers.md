# Devlog T2.6: System Memory (`/proc/meminfo`) & CPU (`/proc/stat`) Parsers

## Task
Implement parsers for system-wide memory metrics (`/proc/meminfo`) and aggregate CPU jiffies (`/proc/stat`), define domain structs `SystemMemory` and `SystemCpuTimes`, and verify with unit tests on synthetic fixtures.

## What I did
- Defined `struct SystemMemory` in `src/proc_reader.hpp` with fields: `total_kb`, `free_kb`, `available_kb`, `buffers_kb`, `cached_kb`, `swap_total_kb`, `swap_free_kb`, along with helper methods: `used_kb()` (using `MemAvailable`), `used_percent()`, `total_mb()`, and `used_mb()`.
- Defined `struct SystemCpuTimes` in `src/proc_reader.hpp` modeling aggregate CPU jiffies (`user`, `nice`, `system`, `idle`, `iowait`, `irq`, `softirq`, `steal`, `guest`, `guest_nice`) and timing calculators `total_time()`, `idle_time()`, and `active_time()`.
- Added `ProcReader::read_meminfo()` and `ProcReader::parse_meminfo_stream()` in `src/proc_reader.cpp`.
- Added `ProcReader::read_system_cpu()` and `ProcReader::parse_stat_cpu_stream()` in `src/proc_reader.cpp` matching the first line of `/proc/stat` (`cpu `).
- Created disk fixture files in `tests/fixtures/proc_system/meminfo` and `tests/fixtures/proc_system/stat`.
- Authored unit tests in `tests/test_proc_reader.cpp` verifying stream parsing, arithmetic calculations, and fixture file reads.

## Why I did it this way
- `/proc/meminfo`'s `MemAvailable` (introduced in Linux 3.14) provides the most accurate estimation of available memory without swapping, taking into account page caches, slab reclaimable, and kernel watermarks. Falling back to `MemTotal - (MemFree + Buffers + Cached)` accommodates older kernels or minimal container environments.
- In `/proc/stat`, the first line prefixed with `"cpu "` aggregates all CPU cores, while subsequent lines (`cpu0`, `cpu1`, ...) represent individual cores. Parsing the aggregate line provides the system-wide baseline needed for total CPU time calculations.
- Alternatives considered: Reading only `MemFree`. Rejected because modern Linux kernels aggressively use free memory for disk caching (`Buffers` and `Cached`), so `MemFree` alone severely underestimates available RAM.

## Problems encountered
- Linker error `Undefined symbols for architecture arm64: minimon::ProcReader::parse_meminfo_stream...` occurred during initial test link.
  - *Symptom*: Undefined reference to `parse_meminfo_stream` and `parse_stat_cpu_stream`.
  - *Root Cause*: The new parser implementations were appended after the closing `} // namespace minimon` in `src/proc_reader.cpp`, causing them to be compiled into the global namespace rather than `minimon::ProcReader`.

## How I solved them
- Moved `} // namespace minimon` to the very bottom of `src/proc_reader.cpp`, enclosing the newly added member functions within the `minimon` namespace.

## Linux concepts involved
- Virtual Memory architecture: difference between free memory (`MemFree`) and genuinely available memory (`MemAvailable`), buffer cache for raw disk blocks, and page cache for files.
- Aggregate CPU accounting: jiffies spent in user mode, nice mode, system (kernel/syscalls), idle, and I/O wait (`iowait`).

## How to verify
```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

## Open questions
- None. Next task is T2.7: two-sample CPU tracker module and PID-reuse detection.
