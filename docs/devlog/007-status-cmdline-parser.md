# Devlog T2.4: `/proc/[pid]/status` & `cmdline` Parsers

## Task
Implement parsers for `/proc/[pid]/status` (extracting process name, UID, VmRSS, VmSize, and thread count) and `/proc/[pid]/cmdline` (converting NUL-separated argument lists into command strings), and verify with unit tests on synthetic fixtures.

## What I did
- Added `ProcReader::read_status(pid_t pid, Process& out)` and `ProcReader::parse_status_stream(std::istream& is, Process& out)` in `src/proc_reader.hpp` and `src/proc_reader.cpp`:
  - Scans line-by-line for key prefixes: `Name:`, `State:`, `Uid:`, `VmSize:`, `VmRSS:`, and `Threads:`.
  - Extracts the real UID from the first column of `Uid:` for subsequent username resolution.
  - Safely handles kernel threads where `VmSize` and `VmRSS` lines are not present, leaving memory metrics at 0 kB.
- Added `ProcReader::read_cmdline(pid_t pid, Process& out)` and `ProcReader::parse_cmdline_buffer(const char* buf, size_t length)`:
  - Reads binary contents of `/proc/[pid]/cmdline`.
  - Replaces internal NUL delimiters (`\0`) with spaces while trimming trailing delimiters.
  - Gracefully handles empty command line files (typical for kernel threads like `kthreadd` or zombie processes).
- Created fixture directories in `tests/fixtures/proc_status_cmdline/` for both standard user processes (PID 500) and kernel threads (PID 2).
- Added comprehensive unit tests in `tests/test_proc_reader.cpp`.

## Why I did it this way
- `/proc/[pid]/status` provides human-readable memory statistics (in kB) and multi-column UID/GID mappings, making it a reliable source for VmRSS, VmSize, and the owning UID.
- Reading `cmdline` in binary mode is necessary because standard C/C++ string functions (like `strlen`, `strcpy`, or `operator>>`) terminate prematurely upon encountering the first NUL byte between command line arguments.
- Alternatives considered: Using `strtok` on `cmdline`. Rejected because `strtok` modifies the buffer in-place and is not thread-safe.

## Problems encountered
- `ProcCmdlineTest.ParseNulSeparatedBuffer` test failed on initial run.
  - *Symptom*: Expected `"minimon --search chrome --top 10"`, but received `"minimon --search chrome --top\b"`.
  - *Root Cause*: In the test string literal `"minimon\0--search\0chrome\0--top\010\0"`, C++ interpreted `\010` as an octal escape sequence (octal 10 = decimal 8, ASCII backspace `\b`) rather than a NUL byte followed by ASCII `"10"`.

## How I solved them
- Broke the string literal into adjacent string literal fragments: `"minimon\0" "--search\0" "chrome\0" "--top\0" "10\0"`. Adjacent string literals are concatenated by the preprocessor without treating `\0` and `10` as a single escape sequence.

## Linux concepts involved
- Virtual memory accounting: `VmSize` (virtual address space allocated) vs `VmRSS` (physical RAM pages resident in memory).
- Kernel threads: execution contexts managed entirely in kernel space with no userspace memory maps (no `VmSize`/`VmRSS`) and an empty `cmdline`.
- Argument passing: the Linux kernel stores command line arguments passed to `execve(2)` as a contiguous sequence of NUL-terminated C strings in the process's address space.

## How to verify
```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

## Open questions
- None. Next task is T2.5: username lookup from UID (`getpwuid` with caching).
