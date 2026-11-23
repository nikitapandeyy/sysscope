# Devlog T2.1: Test Framework & Core Library Target

## Task
Configure the `minimon_core` static library target, integrate GoogleTest via CMake `FetchContent`, define the core `Process` data structure, and verify the first passing unit tests with CTest.

## What I did
- Updated `CMakeLists.txt` to:
  - Define `minimon_core` static library target including `src/process.cpp`.
  - Fetch GoogleTest v1.14.0 via CMake `FetchContent` with `CMP0135` policy set to `NEW` and `DOWNLOAD_EXTRACT_TIMESTAMP TRUE`.
  - Create `minimon_tests` linking `minimon_core` and `GTest::gtest_main`, registered with `gtest_discover_tests`.
- Created `src/process.hpp` declaring `struct Process` with fields: `pid`, `ppid`, `name`, `state`, `user`, `uid`, `cpu_percent`, `rss_kb`, `vm_size_kb`, `mem_percent`, `threads`, `starttime`, `cmdline`, `utime`, `stime`, and convenience method `rss_mb()`.
- Created `src/process.cpp` as the translation unit for core process logic.
- Authored `tests/test_trivial.cpp` with unit tests checking default initialization and RSS kB-to-MB conversion.
- Verified that all unit tests pass under `ctest`.

## Why I did it this way
- Placing the domain struct in `src/process.hpp` and linking it via `minimon_core` allows both the main executable (`minimon`) and the test suite (`minimon_tests`) to consume identical data models without code duplication.
- Using GoogleTest with `gtest_discover_tests` enables automatic test discovery so each `TEST()` macro becomes an individual test case in CTest.
- Alternatives considered: Using git submodules for GoogleTest. Rejected because `FetchContent` makes the repository self-contained and avoids submodule sync hurdles for users.

## Problems encountered
- Initial CMake configuration failed while downloading GoogleTest archive from GitHub.
  - *Symptom*: `error: downloading 'https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz' failed: Couldn't resolve host name`
  - *Root Cause*: Standard sandbox isolates network connectivity; downloading external dependencies during CMake configure requires network access.
- CMake warning regarding `CMP0135` download extract timestamps.
  - *Symptom*: Author warning about `DOWNLOAD_EXTRACT_TIMESTAMP`.
  - *Root Cause*: CMake 3.24+ requires explicit timestamp policy for URL downloads.

## How I solved them
- Re-ran the configuration with `BypassSandbox: true` so CMake could fetch the GoogleTest archive.
- Set `cmake_policy(SET CMP0135 NEW)` and `DOWNLOAD_EXTRACT_TIMESTAMP TRUE` in `FetchContent_Declare`, eliminating the warning cleanly.

## Linux concepts involved
- Process state modeling: mapping POSIX and kernel concepts (`pid_t`, `uid_t`, VmRSS memory in kilobytes, execution states) into C++ structures.
- Test discovery: executing test suites via POSIX process invocation in CTest.

## How to verify
```bash
cmake -B build -S .
cmake --build build
ctest --test-dir build --output-on-failure
```

## Open questions
- None. Next task is T2.2: PID enumeration in the proc root.
