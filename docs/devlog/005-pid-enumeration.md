# Devlog T2.2: PID Enumeration

## Task
Implement PID enumeration in `src/proc_reader.cpp` to discover all active process IDs by inspecting numeric subdirectories in the configured proc root, and verify with unit tests on synthetic fixtures.

## What I did
- Created `src/proc_reader.hpp` declaring `ProcReader` with a configurable `proc_root` (default `"/proc"`).
- Implemented `ProcReader::get_pids()` in `src/proc_reader.cpp` using `std::filesystem::directory_iterator` with `std::filesystem::directory_options::skip_permission_denied` and `std::error_code`:
  - Filters entries to ensure only directories whose filenames consist exclusively of ASCII digits are considered.
  - Converts directory names to `pid_t` and sorts the resulting list in ascending numerical order.
  - Safely returns an empty list if the directory cannot be opened (e.g. non-existent or lacking permissions).
- Created a synthetic fixture directory tree in `tests/fixtures/proc_enumeration/` containing numeric directories (`1/`, `42/`, `999/`), non-numeric directories (`not_a_pid/`, `self/`, `sys/`), and regular files (`uptime`, `stat`).
- Added compile-time definition `TEST_FIXTURES_DIR` in `CMakeLists.txt` and authored unit tests in `tests/test_proc_reader.cpp` verifying directory scanning.

## Why I did it this way
- In Linux, every process running on the host is represented as a directory in `/proc` named after its PID. Filtering out non-numeric directories like `/proc/sys`, `/proc/bus`, `/proc/net`, `/proc/self` is the standard, canonical mechanism for discovering active processes.
- Using `std::filesystem::directory_iterator` with error codes avoids exceptions and guarantees clean resource management (automatically closing directory handles).
- Alternatives considered: Using `opendir()` and `readdir()` with raw POSIX pointers. Rejected because `std::filesystem` is standard C++17, RAII-safe, and less prone to leaked `DIR*` handles.

## Problems encountered
- `ProcReaderTest.EnumeratePidsFromFixture` failed on initial run because CTest executes binaries with working directory set to `build/` rather than the repository root.
  - *Symptom*: Expected 3 PIDs, received 0.
  - *Root Cause*: Relative path `tests/fixtures/proc_enumeration` resolved to `build/tests/fixtures/proc_enumeration` which does not exist.

## How I solved them
- Added `target_compile_definitions(minimon_tests PRIVATE TEST_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures")` in `CMakeLists.txt` to pass the absolute fixture path, and set `WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}` in `gtest_discover_tests`.

## Linux concepts involved
- The `/proc` directory structure: processes are exposed as numeric directory entries (`/proc/[pid]`), while system-wide metrics and control files are named strings (`/proc/stat`, `/proc/sys`).
- File descriptor and directory stream safety when iterating dynamic kernel pseudo-filesystems.

## How to verify
```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

## Open questions
- None. Next task is T2.3: `/proc/[pid]/stat` parser with tests for complex `comm` names.
