# Devlog T3.1: Search & Filter Module

## Task
Implement the process search and filtering module in `src/filter.hpp` and `src/filter.cpp` supporting case-insensitive substring searching (process name and command line), exact PID matching, user name matching, execution state filtering, and minimum CPU% / memory thresholds.

## What I did
- Authored `src/filter.hpp` declaring:
  - `struct FilterOptions`: holding `name_search`, `pid` (`std::optional<pid_t>`), `user`, `state` (`char`), `min_cpu` (`double`), and `min_mem_mb` (`double`).
  - `case_insensitive_contains()` and `case_insensitive_equals()` string helpers.
  - `matches_filter()` and `apply_filter()`.
- Implemented `src/filter.cpp`:
  - `case_insensitive_contains`: uses `std::search` with `std::tolower` to match substrings across character encodings without allocating transformed strings.
  - `matches_filter`: evaluates all specified criteria in `FilterOptions` using conjunction (logical AND). Checks both `proc.name` and `proc.cmdline` for substring matches.
  - `apply_filter`: preserves matching processes and reserves capacity up-front to minimize vector reallocations.
- Updated `CMakeLists.txt` to include `src/filter.cpp` in `minimon_core` and set `DISCOVERY_MODE PRE_TEST` in `gtest_discover_tests`.
- Authored 10 unit tests in `tests/test_filter.cpp` testing string helpers, individual filter fields, and combined criteria.

## Why I did it this way
- Users frequently search by partial names (e.g. `--search chrome` should match `/opt/google/chrome/chrome` as well as the short name `chrome`). Searching both `name` (from `stat`/`status`) and `cmdline` provides intuitive search semantics similar to `pgrep -f`.
- Performing case-insensitive comparisons on the fly using character iterators avoids allocating temporary lowercase strings on every candidate process.
- Alternatives considered: Using `std::regex`. Rejected because users expect simple plain-text substring searches, and `std::regex` compilation incurs noticeable latency when filtering large process lists.

## Problems encountered
- Build error during CMake test discovery: `GoogleTestAddTests.cmake:235 (message): Error running test executable ... Timeout: '5' ... sh: /bin/ps: Operation not permitted`.
  - *Symptom*: Build step failed while CMake attempted to execute `minimon_tests` post-link to discover test names.
  - *Root Cause*: By default, CMake's `gtest_discover_tests` runs the test executable synchronously at build time with a strict 5-second timeout. In sandboxed environments or under heavy load, binary startup can exceed 5 seconds, triggering an internal timeout where CMake attempts to run `/bin/ps` to kill the subprocess.

## How I solved them
- Configured `DISCOVERY_MODE PRE_TEST` and `DISCOVERY_TIMEOUT 30` in `gtest_discover_tests`. In `PRE_TEST` mode, CMake defers test discovery until `ctest` runs rather than executing the binary during compilation.

## Linux concepts involved
- Process matching semantics: differentiating between executable names (`task_struct->comm`, max 16 chars) and full command line arguments stored in the process's initial stack frame (`argv`).
- Process states: querying single-character process status flags (`R`, `S`, `D`, `Z`, `T`).

## How to verify
```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

## Open questions
- None. Next task is T3.2: sorting module (by CPU, mem, PID, name; ascending/descending).
