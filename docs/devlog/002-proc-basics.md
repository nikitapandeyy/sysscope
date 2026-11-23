# Devlog T1.2: `/proc` Basics Documentation

## Task
Document the core principles of the Linux `/proc` filesystem, process abstractions (PID/TGID), execution states, and timing concepts (jiffies, clock ticks), including real sample outputs for `/proc/self/stat`, `/proc/self/status`, `/proc/meminfo`, and `/proc/stat` with field-by-field annotations.

## What I did
- Authored `docs/learning/proc-basics.md` covering:
  - The mechanism of `procfs` as a dynamic, kernel-synthesized pseudo-filesystem.
  - The architectural difference between POSIX userspace PIDs and kernel `task_struct` TGIDs/TIDs.
  - The standard process execution states (`R`, `S`, `D`, `Z`, `T`, `t`).
  - Timer interrupts, jiffies, and POSIX `_SC_CLK_TCK` timing math.
  - Precise field-by-field breakdown of `/proc/[pid]/stat`, `/proc/[pid]/status`, `/proc/meminfo`, and `/proc/stat`.
  - Explanations of tricky edge cases, such as whitespace and parentheses in the `comm` field of `/proc/[pid]/stat`.

## Why I did it this way
- Documenting the exact fields before writing parser code ensures that all team members and mentors can understand the semantic meaning of every field extracted by `minimon`.
- Highlights the critical edge case of process name parsing (`comm` field bounded by `(` and `)`) where naive whitespace tokenization fails if a command contains spaces or nested parentheses.
- Alternatives considered: Relying only on inline code comments. Rejected because a dedicated learning guide provides full context, complete sample outputs, and the mathematical formulas for CPU% and memory% calculations.

## Problems encountered
- On the macOS host machine, native `/proc` is not present (macOS kernel XNU uses Mach APIs/libproc).
  - *Symptom*: `ls /proc` reports `No such file or directory`.
  - *Root Cause*: macOS does not implement the Linux `procfs` VFS layer.

## How I solved them
- Used standard Linux 6.x kernel samples captured from Linux environments to document the exact byte formats.
- Confirmed the design pattern for `minimon`: `proc_reader` will accept a configurable root path (`/proc` by default, or injected fixture paths) so all parsing and unit tests run cleanly and reliably on any POSIX developer environment.

## Linux concepts involved
- Virtual Filesystems (VFS) and procfs inode synthesis.
- `struct task_struct`, TGID vs TID, and POSIX `getpid()` semantics.
- Process states: `R`, `S`, `D` (uninterruptible disk sleep), `Z` (zombie waiting for `waitpid(2)`).
- Time management: `USER_HZ`, `sysconf(_SC_CLK_TCK)` clock tick frequency, and utime/stime counters.

## How to verify
Read `docs/learning/proc-basics.md` to verify all required fields and explanations:
```bash
cat docs/learning/proc-basics.md
```

## Open questions
- In Task T1.3, create a standalone C++ experiment program in `experiments/` demonstrating parsing of `/proc/self/stat`.
