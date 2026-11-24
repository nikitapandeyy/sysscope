# Mini Process Monitor (`minimon`)
> **Linux System Intelligence & Process Monitor in Modern C++17**

[![CI](https://github.com/nikitapandeyy/sysscope/actions/workflows/ci.yml/badge.svg)](https://github.com/nikitapandeyy/sysscope/actions/workflows/ci.yml)
[![Language](https://img.shields.io/badge/language-C%2B%2B17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS-lightgrey.svg)](https://kernel.org)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

Mini Process Monitor (`minimon`) is a high-performance, educational Linux process inspection tool written from scratch in C++17. It directly queries the Linux kernel's virtual `/proc` filesystem to track process execution, memory consumption, CPU utilization, and system-wide state without external libraries.

Every component is built for clarity, correctness, and defensive systems engineering — robust against terminating processes, PID reuse, and adversarial command names.

---

## Architecture Overview

```
                      +-----------------------------+
                      |   Linux Kernel /proc VFS    |
                      +-----------------------------+
                        |        |         |       |
      /proc/[pid]/stat -+        |         |       +- /proc/stat
    /proc/[pid]/status ----------+         +- /proc/meminfo
   /proc/[pid]/cmdline --------------------+
                                 |
                                 v
                 +-------------------------------+
                 |     ProcReader (procfs)       |
                 | - Numeric PID enumeration     |
                 | - comm parsing from last ')'  |
                 | - Configurable root for tests |
                 +-------------------------------+
                                 |
                                 v
      +-------------------+      |
      |    CpuTracker     | <----+
      | - 2-sample deltas |      |
      | - PID reuse check |      |
      +-------------------+      |
               |                 v
               |       +--------------------+
               +-----> |   Process Vector   |
                       +--------------------+
                                 |
                                 v
                       +--------------------+
                       |    Filter & Sort   |
                       | - Name, PID, User  |
                       | - Min CPU%, Memory |
                       | - Stable Multi-Sort|
                       +--------------------+
                                 |
                                 v
                     +------------------------+
                     |    Display & Output    |
                     +------------------------+
                     /                        \
                    v                          v
       +-----------------------+     +-----------------------+
       |   Table Renderer      |     |     JSON Serializer   |
       | - Tasks/Mem/Load banner|     | - RFC 8259 escaping   |
       | - Dynamic terminal ioctl|    | - Schema v1.0         |
       | - ANSI live refresh   |     | - Streamable to jq    |
       +-----------------------+     +-----------------------+
```

---

## Key Features

1. **Robust `/proc` Parsing**:
   - `/proc/[pid]/stat`: Accurately handles process names (`comm`) containing spaces, braces, or nested parentheses by parsing backwards from the last `)` delimiter.
   - `/proc/[pid]/status`: Extracts `Uid`, `VmRSS` (Resident Set Size), `VmSize` (Virtual Memory Size), execution `State`, and thread counts.
   - `/proc/[pid]/cmdline`: Decodes binary NUL-separated argument buffers, gracefully handling kernel threads with bracketed names (e.g. `[kthreadd]`).
   - `/proc/meminfo` & `/proc/stat`: Ingests aggregate physical RAM, swap usage, and CPU jiffies.
2. **Defensive Concurrency & Race-Free Lifecycle**:
   - Gracefully handles processes that terminate mid-monitoring (`ENOENT` / `ESRCH`) without crashing.
   - Prunes departed processes from tracking maps.
3. **Accurate Two-Sample CPU% Math**:
   - Computes utilization using deltas of `utime + stime` divided by elapsed wall-clock time scaled by `sysconf(_SC_CLK_TCK)`.
   - Detects PID reuse via `/proc/[pid]/stat` field 22 (`starttime`).
4. **Search, Filter & Sorting**:
   - Case-insensitive substring search across command lines.
   - Exact filtering by PID, effective username, process state (`R`, `S`, `D`, `Z`, `T`), minimum CPU%, or minimum RAM.
   - Multi-field sorting by CPU%, memory (MB), PID, or process name in ascending or descending order.
5. **Interactive TUI & JSON Interfaces**:
   - Real-time live refresh mode with ANSI cursor positioning (`\033[H`) to eliminate flicker.
   - Terminal geometry adaptation via `ioctl(TIOCGWINSZ)` to prevent line wrapping.
   - Safe signal handling (`sigaction`) and RAII terminal restoration (`TerminalGuard`).
   - RFC 8259-compliant JSON output mode (`--json`) for automated pipeline ingestion.

---

## CPU Utilization Math

Process CPU utilization cannot be measured instantaneously; it represents the rate of CPU ticks consumed over an elapsed time window:

$$\Delta \text{ticks} = (\text{utime}_2 + \text{stime}_2) - (\text{utime}_1 + \text{stime}_1)$$

$$\Delta t_{\text{sec}} = \text{duration\_cast}(t_2 - t_1)$$

$$\text{CPU\%} = \left( \frac{\Delta \text{ticks}}{\Delta t_{\text{sec}} \times \text{sysconf}(\_SC\_CLK\_TCK)} \right) \times 100.0$$

- **Jiffies / Clock Ticks**: `utime` (user space) and `stime` (kernel space) are expressed in clock ticks. On Linux x86/ARM, `_SC_CLK_TCK` is almost universally 100 ticks per second (1 tick = 10ms).
- **PID Reuse Detection**: If the kernel reuses a PID between samples, a naive delta would yield erroneous or negative percentages. `CpuTracker` stores field 22 (`starttime` in ticks since system boot) along with the PID. If `starttime` changes, the tracker resets baseline history.
- **Counter Anomaly Guards**: If $\Delta \text{ticks} < 0$ or $\Delta t \le 0$, the tracker resets the sample and reports `0.0%`.

---

## Command-Line Usage

```bash
# Live interactive dashboard (top 20 processes by CPU, refreshes every 1s)
minimon

# Print single-shot snapshot and exit immediately
minimon --once

# Search for processes containing 'chrome'
minimon --search chrome

# Filter sleeping root processes
minimon --user root --state S

# Show top 10 memory-consuming processes
minimon --sort mem --top 10

# Filter processes using at least 5% CPU and 100 MB RAM
minimon --min-cpu 5.0 --min-mem 100.0

# Export structured JSON for tooling or jq pipelines
minimon --json | jq '.processes[] | select(.cpu_percent > 10.0)'

# Inspect custom procfs mount or test fixture
minimon --proc-root /tmp/custom_proc --once
```

---

## Sample Output

### Terminal Table View (`minimon --once --top 5`)
```text
Tasks: 184 total, 2 running, 180 sleeping, 0 zombie, 2 stopped
Mem:   16384.0M total, 5842.1M used (35.7%), 10541.9M avail, 240.5M/2048.0M swap
Load:  1.42, 1.15, 0.98

    PID    PPID USER       STAT   %CPU   %MEM       RSS  THR COMMAND
--------------------------------------------------------------------------------
   1042       1 root       S       8.4    3.2    524.3M   14 /usr/bin/dockerd
    412       1 systemd    S       1.2    0.2     32.8M    1 /lib/systemd/systemd-journald
   5812    5800 nikita     R      98.6    1.8    294.9M    8 /usr/bin/clang++ -O3
   1820       1 root       S       0.4    0.5     81.9M    4 /usr/sbin/NetworkManager
      1       0 root       S       0.0    0.1     16.4M    1 /sbin/init
```

### JSON Export (`minimon --json --top 1`)
```json
{
  "schema_version": "1.0",
  "system": {
    "tasks": {
      "total": 184,
      "running": 2,
      "sleeping": 180,
      "zombie": 0,
      "stopped": 2
    },
    "memory": {
      "total_kb": 16777216,
      "used_kb": 5982310,
      "free_kb": 4194304,
      "available_kb": 10794906,
      "swap_total_kb": 2097152,
      "swap_used_kb": 246272,
      "used_percent": 35.66
    },
    "loadavg": [1.42, 1.15, 0.98]
  },
  "processes": [
    {
      "pid": 5812,
      "ppid": 5800,
      "name": "clang++",
      "state": "R",
      "user": "nikita",
      "uid": 1000,
      "cpu_percent": 98.6,
      "rss_kb": 302000,
      "vm_size_kb": 850000,
      "mem_percent": 1.80,
      "threads": 8,
      "cmdline": "/usr/bin/clang++ -O3"
    }
  ]
}
```

---

## Building and Testing

### Prerequisites
- C++17 compliant compiler (`gcc` 9+, `clang` 10+, or AppleClang)
- CMake 3.16+
- POSIX-compliant operating system (Linux or macOS)

### Standard Build
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

### Running Unit and Integration Tests
```bash
ctest --test-dir build --output-on-failure
```

### Build with Sanitizers (ASan + UBSan)
```bash
cmake -B build -DENABLE_SANITIZERS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

### Valgrind Memory Leak Check (Linux)
```bash
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1 \
  ./build/minimon --proc-root tests/fixtures/proc_full --once
```

---

## Documentation Index

- [Linux /proc Fundamentals & Telemetry Guide](docs/learning/proc-basics.md): Virtual files, PID vs TGID, jiffies, process states, and `/proc` field specifications.
- [JSON Export Specification v1.0](docs/learning/json-format.md): Telemetry schema, field mappings, and pipeline integration.
- [Code Quality & Sanitizer Report](docs/quality.md): Dynamic sanitization results, Valgrind analysis, and memory safety hygiene.
- [GDB Debugging Walkthrough](docs/gdb-session.md): Real-world GDB session transcript stepping through `/proc/[pid]/stat` parsing and core dump diagnosis.
- [Task Devlog Index](docs/devlog/README.md): Detailed changelog of every milestone and task.
