# Devlog Task 018: End-to-End Integration Testing with Real Child Workloads

## Task
Implement end-to-end integration tests that spawn real child processes executing controlled CPU-intensive and memory-intensive workloads, measure their telemetry via `minimon` / POSIX APIs, and verify full CLI pipeline execution.

---

## What I Did
1. **Workload Generator (`tests/workload_child.cpp`)**:
   - Created a standalone test workload binary `test_workload` that accepts `--cpu` (tight arithmetic loop burning CPU cycles) and `--mem <MB>` (allocating dynamic heap memory and writing across all pages with `std::memset` to ensure they are resident in physical RAM / RSS).
   - Implemented POSIX pipe-based synchronization (`READY\n` emitted to stdout) so the parent test process knows exactly when memory pages are faulted or CPU spinning has commenced before taking samples.
   - Handled `SIGTERM` via `sigaction` for clean process shutdown.
2. **RAII Process Lifespan Guard (`tests/test_integration.cpp`)**:
   - Implemented `WorkloadChildGuard` class managing `fork()`, `execv()`, stdout pipe redirection, and deterministic cleanup with `kill(pid, SIGTERM)` and `waitpid()`.
3. **Single Process Telemetry API (`src/proc_reader.hpp`, `src/proc_reader.cpp`)**:
   - Added `minimon::ProcReader::read_single_process(pid_t pid, Process& out) const` to encapsulate reading `stat`, `status`, `cmdline`, and UID-to-username resolution for a specific process ID.
   - Refactored `read_all_processes()` to reuse `read_single_process()`.
4. **Integration Test Suite (`tests/test_integration.cpp`)**:
   - `ChildMemoryAllocationWorkload`: Spawns 25 MB child workload and verifies resident set size bounds on Linux (or child lifecycle synchronization on Darwin).
   - `ChildCpuBurnerWorkload`: Spawns CPU-burning child and tests two-sample delta CPU% calculation.
   - `CliBinaryEndToEndExecution`: Executes `minimon --once` end-to-end via POSIX `popen` against full procfs fixtures and verifies output formatting.
   - `CliBinaryFilteringAndJson`: Executes `minimon --json --search bash` verifying JSON emission and filtering.
5. **CMake Configuration (`CMakeLists.txt`)**:
   - Added `test_workload` executable and wired build dependencies and target file paths into `minimon_tests`.

---

## Why I Did It This Way
- **Pipe Synchronization vs. Sleep**: Arbitrary `sleep()` calls introduce flakiness in integration tests. Using a unidirectional pipe where the child writes `"READY\n"` guarantees that `malloc()` and `memset()` are fully committed to physical RAM before parent measurements start.
- **Physical Memory Faulting (`memset`)**: In Linux and modern virtual memory architectures, `malloc()` uses lazy allocation (overcommit) where pages are not mapped to physical frames until written to. Without `memset()`, the process RSS (`VmRSS`) remains negligible regardless of the allocated size.
- **Cross-Platform Host Gracefulness**: On macOS host development machines where `/proc` is not mounted, the test validates child process spawning, pipe IPC, and CLI execution against full mock fixtures, while seamlessly executing real `/proc` inspection in Linux environments (such as CI).

---

## Problems Encountered
- **Problem**: When invoking `reader.read_single_process()` in the initial implementation of `test_integration.cpp`, compilation failed with `no member named 'read_process' in 'minimon::ProcReader'`.
  - **Symptom**: `CMakeFiles/minimon_tests.dir/tests/test_integration.cpp.o: error: no member named 'read_process'`.
  - **Root Cause**: The method was initially planned under the name `read_single_process` in `ProcReader` header, but called as `read_process` in `test_integration.cpp`.
  - **Solution**: Aligned the call in `test_integration.cpp` to `read_single_process()`.

---

## Linux/OS Concepts Involved
- **Demand Paging and RSS**: The kernel creates virtual memory mappings (`vm_area_struct`) without allocating physical page frames immediately. A minor page fault occurs on the first memory write, allocating a physical frame and incrementing `VmRSS` in `/proc/[pid]/status`.
- **Inter-Process Synchronization via Pipes**: `pipe(2)`, `dup2(2)`, and blocking `read(2)` provide deterministic synchronization without timing races.
- **Zombie Prevention**: Terminated child processes remain as zombies in the process table until the parent invokes `waitpid(2)` to collect their termination status.

---

## How to Verify
```bash
# 1. Build targets (including test_workload and minimon_tests)
cmake -B build && cmake --build build

# 2. Run all tests including integration tests
ctest --test-dir build --output-on-failure

# 3. Manually test the workload generator
./build/test_workload --mem 20 --duration 2
```

---

## Open Questions / Follow-ups
- Proceed to Task T4.2: Valgrind and compiler sanitizers (AddressSanitizer and UndefinedBehaviorSanitizer) and author `docs/quality.md`.
