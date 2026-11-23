# Devlog T2.3: `/proc/[pid]/stat` Parser

## Task
Implement the `/proc/[pid]/stat` parser in `src/proc_reader.cpp`, ensuring robust parsing of process names containing spaces and nested parentheses, and verify with unit tests on synthetic fixtures and malformed inputs.

## What I did
- Added `ProcReader::read_stat(pid_t pid, Process& out)` and static helper `ProcReader::parse_stat_string(const std::string& line, Process& out)` in `src/proc_reader.hpp` and `src/proc_reader.cpp`.
- Implemented robust tokenization:
  - Locates the first `(` and the **last** `)` in the stat line to isolate the `comm` string.
  - Extracts the PID from the text preceding the first `(`.
  - Parses remaining positional fields from the substring following the last `)`: `state` (field 3), `ppid` (field 4), `utime` (field 14), `stime` (field 15), `threads` (field 20), and `starttime` (field 22).
  - Skips unused intermediate fields (5-13, 16-19, 21).
  - Returns `false` gracefully if the file cannot be opened (e.g. process exited mid-read, reporting `ENOENT`) or is malformed.
- Created test fixtures under `tests/fixtures/proc_stat/` with standard processes, processes with spaces (`"(a b)"`), and processes with nested parentheses (`"((a) b (c))"`).
- Added comprehensive unit tests in `tests/test_proc_reader.cpp` testing normal strings, edge-case names, file reads, non-existent PIDs, and malformed lines.

## Why I did it this way
- The Linux kernel formats `/proc/[pid]/stat` as space-separated tokens, but the `comm` field is derived from `task_struct->comm` (up to 16 bytes by default, or set via `prctl(PR_SET_NAME)`), which can contain spaces and parentheses. A naive whitespace parser will shift field indices unpredictably. Seeking the last `)` is the canonical, reliable technique used by standard Linux utilities (such as `procps-ng` / `top` / `ps`).
- Exposing `parse_stat_string` as a static helper allows direct in-memory unit testing of edge cases without disk I/O, while `read_stat` handles reading from the filesystem.

## Problems encountered
None. The parsing logic compiled cleanly and passed all 10 unit tests on the first execution.

## How I solved them
N/A (no problems encountered).

## Linux concepts involved
- `/proc/[pid]/stat` field layout: positional fields according to `man 5 proc`.
- Process timing fields: `utime` (clock ticks in user space), `stime` (clock ticks in kernel space), `starttime` (ticks after system boot).
- File disappearance: processes exiting between directory enumeration and reading the `stat` file return `ENOENT`.

## How to verify
```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

## Open questions
- None. Next task is T2.4: `/proc/[pid]/status` parser (name, UID, VmRSS, VmSize, threads) and `cmdline` reader.
