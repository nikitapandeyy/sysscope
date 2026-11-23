# Devlog T2.8: Unified Process Listing & Vanished Process Handling

## Task
Implement the unified `ProcReader::read_all_processes()` pipeline integrating PID enumeration, stat/status/cmdline readers, system memory accounting, CPU utilization calculation, and graceful skipping of processes that terminate mid-read.

## What I did
- Added `ProcReader::read_all_processes(CpuTracker& cpu_tracker)` in `src/proc_reader.hpp` and `src/proc_reader.cpp`:
  - Discovers all active PIDs using `get_pids()`.
  - Reads system memory from `/proc/meminfo` to calculate `proc.mem_percent` relative to total RAM.
  - Iterates through each PID:
    1. Reads `/proc/[pid]/stat`. If the process terminated before or during read (`ENOENT`), skips it silently without crashing.
    2. Reads `/proc/[pid]/status`. If unreadable, skips it.
    3. Reads `/proc/[pid]/cmdline`. For kernel threads or processes with empty command lines, formats fallback command name `[comm]`.
    4. Calculates `proc.mem_percent`.
    5. Updates per-process CPU% using `cpu_tracker.update_process_cpu()`.
  - Prunes terminated processes from `CpuTracker`'s historical sample store via `cpu_tracker.prune_exited()`.
- Created comprehensive test fixture `tests/fixtures/proc_full/` containing system files, standard processes (PID 1, 100), kernel threads (PID 2), and an empty directory (PID 999) simulating a process that exited immediately after PID discovery.
- Authored unit test `ProcFullScanTest.ReadAllProcessesFromFixture` in `tests/test_proc_reader.cpp` verifying that all valid processes are read, PID 999 is skipped without error, and CPU and memory metrics are accurate.

## Why I did it this way
- Race conditions are inherent to process monitoring on Linux: a process can terminate at any nanosecond between `get_pids()` directory enumeration and opening individual pseudo-files in `/proc/[pid]`. Treating `ENOENT` (and `EACCES` for unprivileged inspection) as normal expected conditions ensures the monitor never crashes, hangs, or corrupts its output table.
- Supplying a bracketed fallback `[name]` for empty `cmdline` files follows standard UNIX conventions (e.g. `ps -ef`, `top`, `htop`), allowing users to immediately identify kernel threads (such as `[kthreadd]` or `[kworker]`).
- Alternatives considered: Throwing exceptions on missing `/proc` entries. Rejected because process termination is a normal, frequent event in an operating system, not an exceptional error.

## Problems encountered
- Compilation warning `-Wunused-result` on `read_cmdline(pid, p)`.
  - *Symptom*: Warning about ignoring nodiscard return value.
  - *Root Cause*: `read_cmdline` was declared with `[[nodiscard]]`, but its return value was not inspected because empty `cmdline` is a valid condition.
- Incomplete type compiler error in `test_proc_reader.cpp`.
  - *Symptom*: `variable has incomplete type 'minimon::CpuTracker'`.
  - *Root Cause*: `test_proc_reader.cpp` did not include `cpu_tracker.hpp`.

## How I solved them
- Cast `read_cmdline` invocation to `static_cast<void>` and added `#include "cpu_tracker.hpp"` in `test_proc_reader.cpp`.

## Linux concepts involved
- Process lifecycle concurrency: processes dynamically forking, running, and calling `exit(2)` concurrently with monitoring tools.
- Kernel thread conventions: tasks spawned by `kthreadd` (PID 2) that run solely in kernel space without a userspace address space or command line arguments.
- Resident memory accounting: calculating process RAM usage against hardware physical memory.

## How to verify
```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

## Open questions
- None. Phase 2 core implementation is complete! Next is Phase 3: search, filter, and table display.
