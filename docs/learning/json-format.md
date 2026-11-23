# Mini Process Monitor (minimon) JSON Export Specification

`minimon --json` provides structured, machine-parseable JSON output representing system health and process states. This document details the schema, data types, and integration patterns.

---

## 1. Design Principles

1. **Self-Describing**: Includes `schema_version` to allow downstream pipelines to handle format changes backwards-compatibly.
2. **Strict Escaping**: All string fields (such as process names and command lines) are sanitized for JSON control characters (`"`, `\`, `\b`, `\f`, `\n`, `\r`, `\t`, and arbitrary `\u00xx` unicode control sequences).
3. **Structured Hierarchy**: Clear division between system-wide telemetry (`system`) and per-process telemetry (`processes`).
4. **No External Dependencies**: Formatted using standard C++17 stream formatting to keep the core binary small, zero-overhead, and dependency-free.

---

## 2. Schema Specification (v1.0)

### Top-Level Object
| Key | Type | Description |
| :--- | :--- | :--- |
| `schema_version` | String | Fixed version string (currently `"1.0"`). |
| `system` | Object | System-wide statistics (tasks, memory, load average). |
| `processes` | Array | List of running process objects. |

### `system` Object
- **`tasks`**:
  - `total` (integer): Total number of active processes enumerated.
  - `running` (integer): Count of processes in `R` (Running/Runnable) state.
  - `sleeping` (integer): Count of processes in `S` (Interruptible Sleep) or `D` (Disk/Uninterruptible Sleep) state.
  - `zombie` (integer): Count of processes in `Z` (Zombie) state awaiting reaper.
  - `stopped` (integer): Count of processes in `T` or `t` (Stopped/Traced) state.
- **`memory`**:
  - `total_kb` (integer): Total physical RAM from `MemTotal` in `/proc/meminfo`.
  - `used_kb` (integer): Derived active memory (`total_kb - available_kb`).
  - `free_kb` (integer): Completely unallocated memory (`MemFree`).
  - `available_kb` (integer): Memory available for starting new applications without swapping (`MemAvailable`).
  - `swap_total_kb` (integer): Total swap space (`SwapTotal`).
  - `swap_used_kb` (integer): Used swap space (`SwapTotal - SwapFree`).
  - `used_percent` (float): Memory utilization percentage (`used_kb / total_kb * 100.0`), 2 decimal places.
- **`loadavg`**:
  - Array of 3 floats `[1m, 5m, 15m]`: System load average over 1, 5, and 15 minutes.

### `processes` Item Object
| Key | Type | Procfs Source | Description |
| :--- | :--- | :--- | :--- |
| `pid` | Integer | Directory name | Process ID |
| `ppid` | Integer | `/proc/[pid]/stat` field 4 | Parent Process ID |
| `name` | String | `/proc/[pid]/status` `Name:` | Executable name / comm (escaped) |
| `state` | String | `/proc/[pid]/stat` field 3 | Single-character process state (`R`, `S`, `D`, `Z`, `T`, etc.) |
| `user` | String | `/etc/passwd` via `getpwuid_r` | Username of effective UID (or string UID if unresolvable) |
| `uid` | Integer | `/proc/[pid]/status` `Uid:` | Effective User ID |
| `cpu_percent` | Float | `/proc/[pid]/stat` utime+stime | CPU utilization percentage across sample interval |
| `rss_kb` | Integer | `/proc/[pid]/status` `VmRSS:` | Resident Set Size in kilobytes |
| `vm_size_kb` | Integer | `/proc/[pid]/status` `VmSize:` | Total Virtual Memory Size in kilobytes |
| `mem_percent` | Float | Calculated `rss_kb / total_kb * 100` | Percentage of total physical memory used |
| `threads` | Integer | `/proc/[pid]/status` `Threads:` | Number of threads in the process thread group |
| `cmdline` | String | `/proc/[pid]/cmdline` | Full command line with space delimiters (escaped) |

---

## 3. Example JSON Output

```json
{
  "schema_version": "1.0",
  "system": {
    "tasks": {
      "total": 3,
      "running": 0,
      "sleeping": 3,
      "zombie": 0,
      "stopped": 0
    },
    "memory": {
      "total_kb": 16384000,
      "used_kb": 4096000,
      "free_kb": 4096000,
      "available_kb": 12288000,
      "swap_total_kb": 2048000,
      "swap_used_kb": 1024000,
      "used_percent": 25.00
    },
    "loadavg": [4.53, 3.83, 3.51]
  },
  "processes": [
    {
      "pid": 1,
      "ppid": 0,
      "name": "systemd",
      "state": "S",
      "user": "root",
      "uid": 0,
      "cpu_percent": 0.0,
      "rss_kb": 8192,
      "vm_size_kb": 32768,
      "mem_percent": 0.05,
      "threads": 1,
      "cmdline": "/sbin/init"
    },
    {
      "pid": 2,
      "ppid": 0,
      "name": "kthreadd",
      "state": "S",
      "user": "root",
      "uid": 0,
      "cpu_percent": 0.0,
      "rss_kb": 0,
      "vm_size_kb": 0,
      "mem_percent": 0.00,
      "threads": 2,
      "cmdline": "[kthreadd]"
    },
    {
      "pid": 100,
      "ppid": 1,
      "name": "bash",
      "state": "S",
      "user": "1000",
      "uid": 1000,
      "cpu_percent": 0.0,
      "rss_kb": 4096,
      "vm_size_kb": 16384,
      "mem_percent": 0.03,
      "threads": 1,
      "cmdline": "/bin/bash"
    }
  ]
}
```

---

## 4. Usage with CLI Filters and Downstream Tools

`minimon --json` honors all CLI search, filter, and sorting options, making it ideal for automation scripts:

```bash
# Top 5 memory consuming processes in JSON
minimon --json --sort mem --top 5

# Inspect specific process by PID
minimon --json --pid 1234

# Pipeline into jq for monitoring alerts (e.g. check if any process uses > 50% CPU)
minimon --json | jq '.processes[] | select(.cpu_percent > 50.0)'
```
