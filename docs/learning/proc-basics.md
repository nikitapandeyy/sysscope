# Linux `/proc` Basics: Process Introspection & System Telemetry

This guide explains the fundamental concepts behind the Linux `/proc` virtual filesystem, process representations in the Linux kernel, timing abstractions, and the specific fields used by `minimon`.

---

## 1. What is `/proc`?

In UNIX philosophy, "everything is a file". On Linux, the kernel exposes its runtime internal data structures—process tables, hardware status, memory manager statistics, and scheduler metrics—as a hierarchical filesystem mounted at `/proc`.

### Key Characteristics of `procfs`
- **Pseudo-Filesystem**: `/proc` files do not exist on disk and consume zero physical storage.
- **Dynamic VFS Synthesis**: When a userspace program invokes `open()`, `read()`, or `stat()` on `/proc/1/stat`, the kernel's Virtual Filesystem (VFS) intercepts the call and executes a kernel handler function (e.g. `proc_single_show`) that queries `struct task_struct` and formats the text on the fly into a kernel buffer.
- **Zero-Length Files**: Most files in `/proc` report a `st_size` of `0` in `stat(2)` because the kernel does not know the exact byte length until it formats the data during a `read(2)` call.
- **Platform Note**: macOS (Darwin) does not have a `/proc` filesystem; instead, Darwin exposes process metrics via `sysctl(3)`, Mach kernel ports, and `proc_pidinfo(2)`. For portability and testing on non-Linux development machines, `minimon` abstracts `/proc` behind a configurable root path (`/proc` on Linux, and synthetic fixture directories during unit and integration tests).

---

## 2. Processes, Threads, PIDs, and TGIDs

In the Linux kernel scheduler, there is fundamentally only one unit of execution: `struct task_struct`.

- **Tasks**: Every thread in Linux is a distinct task with its own unique identifier inside the kernel.
- **TGID (Thread Group ID)**: When a process spawns threads via `clone(2)` with `CLONE_THREAD`, all threads share the same TGID (the PID of the first thread/main thread).
- **Userspace PID**: What POSIX and userspace call a "PID" (returned by `getpid(2)`) is actually the kernel's **TGID**.
- **Kernel PID**: Inside the kernel, what is called a "PID" is the thread ID (**TID** in userspace, returned by `gettid(2)`).
- **Process Hierarchy**:
  - `/proc/[pid]` represents the thread group / process.
  - `/proc/[pid]/task/` contains a directory for every individual thread belonging to that thread group (`/proc/[pid]/task/[tid]`).

---

## 3. Process States

A process in Linux transitions between several states during its lifecycle, reported as a single character in `/proc/[pid]/stat` (field 3) and expanded in `/proc/[pid]/status`:

| State Character | State Name | Meaning |
|:---------------:|:-----------|:--------|
| **R** | Running / Runnable | Actively executing on a CPU core or sitting in the scheduler's run queue ready to execute. |
| **S** | Interruptible Sleep | Blocked waiting for an event (e.g. I/O completion, a timer, or a socket read). Can be awakened immediately by signals. |
| **D** | Uninterruptible Sleep | Blocked waiting for hardware I/O (typically disk or NFS). Cannot be interrupted by signals, not even `SIGKILL`. High numbers of `D` state processes indicate I/O bottlenecks. |
| **Z** | Zombie / Defunct | Has terminated execution via `exit(2)`, but its exit status has not yet been read by its parent via `waitpid(2)`. Its `task_struct` remains in the kernel process table solely to hold the exit code. |
| **T** | Stopped | Suspended by a job control signal (e.g. `SIGTSTP`, `SIGSTOP` via `Ctrl+Z`). |
| **t** | Tracing Stop | Paused by a debugger using `ptrace(2)`. |
| **I** | Idle | Kernel thread in an idle state (does not contribute to load average). |
| **X** | Dead | Completely removed from the kernel; should never be visible to userspace. |

---

## 4. Time Accounting: Jiffies, Clock Ticks, and CPU Calculations

### What are Jiffies?
A **jiffy** is a tick of the system timer interrupt. The Linux kernel maintains an internal variable, `jiffies_64`, which increments every time the hardware timer interrupt fires.

### Clock Ticks (`_SC_CLK_TCK`)
In user space, CPU time fields in `/proc` are measured in **clock ticks**, defined by the system architecture's user frequency `USER_HZ` (almost universally **100 ticks per second** on Linux, where 1 tick = 10 ms = 0.01 seconds).
A program queries this value at runtime via POSIX:
```cpp
long ticks_per_sec = sysconf(_SC_CLK_TCK); // typically 100
```

### CPU Utilization Math
To compute the CPU utilization percentage of a process, single samples are meaningless because CPU time is a monotonically increasing counter. We must take **two consecutive samples** separated by a real-time interval:

$$\Delta t_{process} = (\text{utime}_2 + \text{stime}_2) - (\text{utime}_1 + \text{stime}_1)$$

$$\Delta t_{wall\_seconds} = \text{timestamp}_2 - \text{timestamp}_1$$

$$\text{CPU \%} = \frac{\Delta t_{process} / \text{ticks\_per\_sec}}{\Delta t_{wall\_seconds}} \times 100.0$$

---

## 5. File Formats and Annotated Field Reference

### 5.1 `/proc/[pid]/stat`
A single-line file containing space-separated positional fields detailing process scheduling, memory, and CPU accounting.

#### Real Sample Output:
```text
3482 (bash) S 3481 3482 3482 34819 3482 4194304 3822 5812 0 1 28 14 0 0 20 0 1 0 184291 16187392 1210 18446744073709551615 94073402441728 94073403565152 140735790483248 0 0 0 0 65536 3670020 12648430 0 0 0 17 0 0 0 0 0 0 94073404505968 94073404523360 94073413861376 140735790487340 140735790487358 140735790487358 140735790489581 0
```

#### Fields Used by `minimon`:
1. **Field 1 (`pid`)**: `3482` — Process ID.
2. **Field 2 (`comm`)**: `(bash)` — Executable name enclosed in parentheses. **Important**: This field can contain spaces and parentheses (e.g. `(Web Content)` or `(test (1))`). Parsers must find the **last** `)` before reading field 3.
3. **Field 3 (`state`)**: `S` — Process execution state character (`R`, `S`, `D`, `Z`, `T`).
4. **Field 4 (`ppid`)**: `3481` — Parent process ID.
14. **Field 14 (`utime`)**: `28` — Amount of time spent executing user-space instructions, measured in clock ticks.
15. **Field 15 (`stime`)**: `14` — Amount of time spent executing kernel system calls on behalf of the process, measured in clock ticks.
20. **Field 20 (`num_threads`)**: `1` — Number of threads running in this process.
22. **Field 22 (`starttime`)**: `184291` — Time the process started after system boot, in clock ticks. Used alongside PID to uniquely detect PID reuse.

---

### 5.2 `/proc/[pid]/status`
Human-readable, line-delimited key-value pairs providing process status and memory metrics.

#### Real Sample Output:
```text
Name:	bash
Umask:	0022
State:	S (sleeping)
Tgid:	3482
Ngid:	0
Pid:	3482
PPid:	3481
TracerPid:	0
Uid:	1000	1000	1000	1000
Gid:	1000	1000	1000	1000
FDSize:	256
Groups:	1000 4 24 27 30 46 110
VmPeak:	   15808 kB
VmSize:	   15808 kB
VmLck:	       0 kB
VmPin:	       0 kB
VmHWM:	    4840 kB
VmRSS:	    4840 kB
RssAnon:    1840 kB
RssFile:    3000 kB
RssShmem:      0 kB
VmData:	    1728 kB
VmStk:	     136 kB
VmExe:	    1096 kB
VmLib:	    2608 kB
VmPTE:	      64 kB
VmSwap:	       0 kB
Threads:	1
```

#### Fields Used by `minimon`:
- **`Name:`**: Executable name string.
- **`State:`**: Single letter state code (e.g. `S` from `S (sleeping)`).
- **`Uid:`**: Four space/tab-separated numbers: Real, Effective, Saved, and Filesystem UID. `minimon` extracts the Real UID (field 1: `1000`) and resolves it to a username via `getpwuid(3)`.
- **`VmRSS:`**: Resident Set Size in kilobytes (`4840 kB`). Represents the exact amount of physical RAM currently mapped to this process.
- **`VmSize:`**: Total Virtual Memory Size in kilobytes (`15808 kB`).
- **`Threads:`**: Number of tasks in the thread group.

---

### 5.3 `/proc/meminfo`
System-wide physical RAM and swap accounting.

#### Real Sample Output:
```text
MemTotal:       16283744 kB
MemFree:         4128912 kB
MemAvailable:   11894210 kB
Buffers:          341200 kB
Cached:          7841296 kB
SwapCached:            0 kB
Active:          5219400 kB
Inactive:        6184200 kB
SwapTotal:       4194300 kB
SwapFree:        4194300 kB
```

#### Fields Used by `minimon`:
- **`MemTotal:`**: Total usable physical RAM (`16283744 kB`). Used as the denominator to calculate per-process memory percentage:
  $$\text{Memory \%} = \frac{\text{VmRSS}}{\text{MemTotal}} \times 100.0$$
- **`MemAvailable:`**: Practical available RAM for allocating new applications without forcing swapping.
- **`MemFree:`**: Raw unallocated RAM.
- **`SwapTotal:`** and **`SwapFree:`**: System swap utilization.

---

### 5.4 `/proc/stat`
System-wide cumulative CPU utilization across all cores.

#### Real Sample Output:
```text
cpu  321849 14820 184912 9481920 41200 8192 4100 0 0 0
cpu0 160920  7410  92450 4740960 20600 4096 2050 0 0 0
cpu1 160929  7410  92462 4740960 20600 4096 2050 0 0 0
```

#### Fields Used by `minimon`:
The first `cpu` line breaks down aggregated CPU time into ticks spent in:
1. `user`: normal processes executing in user mode
2. `nice`: niced processes executing in user mode
3. `system`: processes executing in kernel mode
4. `idle`: twiddling thumbs
5. `iowait`: waiting for I/O to complete
6. `irq`: servicing hardware interrupts
7. `softirq`: servicing software interrupts
8. `steal`: involuntary wait in virtualized environments
9. `guest`: running a guest vCPU
10. `guest_nice`: running a niced guest vCPU
