# Devlog T3.2: Process Sorting Module

## Task
Implement the process sorting module in `src/filter.hpp` and `src/filter.cpp` supporting sorting by CPU utilization, resident memory (RSS), PID, and executable name in both descending and ascending orders.

## What I did
- Declared `enum class SortField { Cpu, Memory, Pid, Name }` and `enum class SortOrder { Descending, Ascending }` in `src/filter.hpp`.
- Added string parser `parse_sort_field()` accepting `"cpu"`, `"mem"`, `"memory"`, `"rss"`, `"pid"`, `"name"`, and `"comm"` (case-insensitive).
- Implemented `sort_processes()` in `src/filter.cpp` using `std::stable_sort` to maintain relative ordering of processes with identical metric values:
  - Supports both ascending and descending modes.
  - Implements deterministic PID tie-breaking for equal primary values.
- Authored 7 unit tests in `tests/test_filter.cpp` verifying string parsing, CPU sorting, memory sorting, PID sorting, and name sorting.

## Why I did it this way
- `std::stable_sort` guarantees that processes with equal CPU or memory utilization retain their relative order between refresh frames, preventing UI row flicker during live display.
- Adding PID as a secondary tie-breaker ensures strict weak ordering and completely deterministic sorting across different platforms and runs.
- Alternatives considered: Using `std::sort`. Rejected because unstable sorting causes identical items to swap places between frames, leading to jittery live views.

## Problems encountered
None. All 47 tests passed on the first run.

## How I solved them
N/A (no problems encountered).

## Linux concepts involved
- Multi-column process table ordering: mimicking `top` (sorted by CPU% descending by default, or memory via `Shift+M`).
- Stable ordering in terminal monitors to prevent visual jitter on rapid updates.

## How to verify
```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

## Open questions
- None. Next task is T3.3: terminal table renderer (aligned columns, system header banner, cmdline truncation).
