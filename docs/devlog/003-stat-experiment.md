# Devlog T1.3: `/proc/self/stat` Experiment

## Task
Build a throwaway C++ experiment in `experiments/read_stat_exp.cpp` to read `/proc/self/stat`, query `sysconf(_SC_CLK_TCK)`, parse key fields, and verify robust parsing of process names containing spaces and nested parentheses.

## What I did
- Authored `experiments/read_stat_exp.cpp`.
- Implemented `parse_stat_line(const std::string& line, StatFields& out)` which:
  1. Locates the first `(` and the **last** `)` in the stat line to extract the full `comm` executable string.
  2. Extracts `pid` from the prefix preceding the first `(`.
  3. Parses remaining positional fields from the string slice following the last `)` using string streams.
  4. Queries POSIX `sysconf(_SC_CLK_TCK)` to compute execution duration in seconds from raw clock ticks.
- Configured `read_stat_exp` as an executable target in `CMakeLists.txt`.
- Tested the program with both a realistic Linux sample string and custom edge-case inputs with nested parentheses and spaces (e.g. `((nested) (process) name)`).

## Why I did it this way
- The `comm` field in `/proc/[pid]/stat` is notorious for breaking naive parsers (such as `scanf("%d %s %c ...")` or `ss >> pid >> comm >> state`) because Linux permits process names to contain spaces and parentheses. Finding `line.rfind(')')` guarantees that all embedded parentheses in the executable name are contained within the `comm` token, allowing reliable positional extraction of fields 3 through 52.
- Alternatives considered: Using regex. Rejected because regex incurs substantial runtime overhead and compilation cost on high-frequency monitoring loops (e.g. iterating over 500+ PIDs every 1-2 seconds). Finding characters via `find`/`rfind` is $O(N)$ with zero memory allocations beyond the extracted string.

## Problems encountered
None. The string parsing and clock tick conversion compiled with zero warnings under `-Wall -Wextra -Wpedantic` and handled all test cases.

## How I solved them
N/A (no problems encountered).

## Linux concepts involved
- `/proc/[pid]/stat` formatting quirks: positional space-delimited fields where field 2 (`comm`) is parenthesized and can contain arbitrary characters.
- POSIX `sysconf(_SC_CLK_TCK)` returning the clock tick frequency (typically 100 Hz on Linux), converting raw ticks of `utime` and `stime` into seconds.
- Process accounting: user-space CPU time (`utime`) vs kernel-space system call execution time (`stime`).

## How to verify
```bash
cmake --build build
./build/read_stat_exp
./build/read_stat_exp "999 ((nested) (process) name) R 100 1 2 3 4 5 6 7 8 9 100 200 11 12 13 14 4 0 500 1 2 3"
```

## Open questions
- None. This parsing logic is ready to be incorporated into `src/proc_reader.cpp` in Phase 2.
