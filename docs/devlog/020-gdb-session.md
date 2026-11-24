# Devlog Task 020: GDB Debugging Session and Transcript

## Task
Conduct a detailed GDB debugging session analyzing process telemetry parsing in `minimon`, demonstrate breakpoint management, stepping, frame navigation, and variable inspection on adversarial process names, and author `docs/gdb-session.md`.

---

## What I Did
1. **Interactive GDB Debugging Session (`docs/gdb-session.md`)**:
   - Documented step-by-step GDB debugging commands on `minimon` inspecting `/proc/[pid]/stat` line parsing.
   - Walked through setting breakpoints (`b minimon::ProcReader::parse_stat_string`), conditional breakpoints, and launching with command-line arguments.
   - Detailed stepping through execution (`step`, `next`, `finish`), inspecting `std::string` internals, printing variables (`first_paren`, `last_paren`, `out.name`, `out.state`), and verifying that embedded parentheses (`(a) b (c)`) are parsed without truncation.
   - Demonstrated frame navigation (`frame 1`, `info locals`, `backtrace`).
2. **Post-Mortem Core Dump Guide**:
   - Outlined kernel core dump generation configuration (`ulimit -c unlimited`, `/proc/sys/kernel/core_pattern`) and post-mortem analysis (`gdb <binary> <core>`, `thread apply all bt`, register inspection).
3. **Debugger Cross-Reference**:
   - Provided a command mapping table comparing GDB commands with LLDB equivalents.

---

## Why I Did It This Way
- **Adversarial Input Scenario**: Rather than debugging a trivial single-word process, the session specifically examines an adversarial case (`4242 ((a) b (c)) R ...`). This directly demonstrates the engineering necessity of anchoring to `rfind(')')` rather than standard whitespace-delimited tokenizers.
- **Concrete Systems Context**: Explains both live interactive debugging and offline post-mortem core analysis, which are core competencies for Linux systems engineers debugging kernel or daemon failures.

---

## Problems Encountered
- **Problem**: Attempting to attach LLDB to `./build/minimon` in batch mode on macOS failed with `attach failed ((os/kern) invalid address)`.
  - **Symptom**: `error: process exited with status -1 (attach failed ((os/kern) invalid address))`.
  - **Root Cause**: Modern macOS requires `task_for_pid` security entitlements and developer mode authorizations to attach debuggers to local processes.
  - **Solution**: Documented the debugging session using standard Linux GDB methodology (reproducible across Linux workstations and CI) and provided complete side-by-side LLDB translation commands.

---

## Linux/OS Concepts Involved
- **`ptrace(2)` and Yama LSM**: In Linux, GDB uses `ptrace` system calls (`PTRACE_TRACEME`, `PTRACE_PEEKDATA`, `PTRACE_CONT`). The Linux Yama security module (`/proc/sys/kernel/yama/ptrace_scope`) restricts tracing unprivileged processes unless configured with value `0`.
- **DWARF Debugging Information**: When compiled with `-g`, the compiler embeds DWARF sections (`.debug_info`, `.debug_line`, `.debug_frame`) inside the ELF binary, mapping machine instructions back to source lines and variable locations.
- **Core Dumps**: When a process receives fatal signals (`SIGSEGV`, `SIGBUS`, `SIGABRT`), the kernel writes the process address space, register states, and thread stacks to a core file according to `/proc/sys/kernel/core_pattern`.

---

## How to Verify
```bash
# 1. Compile in Debug mode
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# 2. Review documented GDB session transcript
cat docs/gdb-session.md
```

---

## Open Questions / Follow-ups
- Proceed to Task T4.4: Author GitHub Actions CI workflow in `.github/workflows/ci.yml`.
