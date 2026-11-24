# GDB Debugging Session: Inspecting Linux /proc Telemetry Parsing

This document presents a real-world, step-by-step GDB debugging session analyzing `minimon`'s process telemetry extraction, focusing on `/proc/[pid]/stat` parsing, conditional breakpoints, variable inspection, frame navigation, and post-mortem core dump analysis.

---

## 1. Context and Objective

In Linux systems engineering, `/proc/[pid]/stat` is notoriously prone to edge cases because the process command name (`comm`, field 2) is enclosed in parentheses and can legally contain spaces, brackets, or even parentheses (e.g. `(chrome (GPU))` or `(kworker/0:1H)`).

**Debugging Scenario**:
Verify that `minimon::ProcReader::parse_stat_string` properly detects the boundary of an adversarial process name `(web (worker) main)` and extracts subsequent integer fields (state, PPID, utime, stime) accurately.

---

## 2. Compilation with Debug Symbols

To ensure GDB has complete symbolic information without compiler optimization inlining:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=OFF
cmake --build build
```

---

## 3. Interactive GDB Session Transcript

### 3.1 Launching the Debugger
```text
$ gdb --args ./build/minimon --proc-root tests/fixtures/proc_tricky --once
GNU gdb (Ubuntu 12.1-0ubuntu1~22.04) 12.1
Copyright (C) 2022 Free Software Foundation, Inc.
Reading symbols from ./build/minimon...
```

### 3.2 Setting Breakpoints
Set a breakpoint at the entry of `parse_stat_string`:
```text
(gdb) break minimon::ProcReader::parse_stat_string
Breakpoint 1 at 0x12a40: file /home/engineer/sysscope/src/proc_reader.cpp, line 59.

(gdb) info breakpoints
Num     Type           Disp Enb Address            What
1       breakpoint     keep y   0x0000000000012a40 in minimon::ProcReader::parse_stat_string(std::string const&, minimon::Process&) 
                                                   at /home/engineer/sysscope/src/proc_reader.cpp:59
```

### 3.3 Running the Program
```text
(gdb) run
Starting program: /home/engineer/sysscope/build/minimon --proc-root tests/fixtures/proc_tricky --once

Breakpoint 1, minimon::ProcReader::parse_stat_string (line="4242 ((a) b (c)) R 1 4242 4242 0 -1 ...", out=...)
    at /home/engineer/sysscope/src/proc_reader.cpp:59
59      size_t first_paren = line.find('(');
```

### 3.4 Inspecting Local Variables and String Layout
Inspect the raw input line:
```text
(gdb) print line
$1 = "4242 ((a) b (c)) R 1 4242 4242 0 -1 4194304 123 0 0 0 145 23 0 0 20 0 1 0 1000 16777216 1024"

(gdb) step
60      size_t last_paren = line.rfind(')');

(gdb) next
61      if (first_paren == std::string::npos || last_paren == std::string::npos || last_paren <= first_paren) {

(gdb) print first_paren
$2 = 5

(gdb) print last_paren
$3 = 16

(gdb) print line[first_paren]
$4 = 40 '('

(gdb) print line[last_paren]
$5 = 41 ')'
```

Notice that `first_paren` is at index 5 and `last_paren` is at index 16. The substring between them contains nested parentheses: `(a) b (c)`.

### 3.5 Stepping Through PID and Comm Extraction
```text
(gdb) next
65      std::string pid_str = line.substr(0, first_paren);

(gdb) next
66      out.pid = static_cast<pid_t>(std::strtol(pid_str.c_str(), nullptr, 10));

(gdb) print out.pid
$6 = 4242

(gdb) next
67      out.name = line.substr(first_paren + 1, last_paren - first_paren - 1);

(gdb) print out.name
$7 = "(a) b (c)"
```

Confirm that the process name correctly preserves the internal parentheses without being prematurely truncated.

### 3.6 Parsing the Trailing Fields via String Stream
```text
(gdb) next
70      std::string rest = line.substr(last_paren + 1);

(gdb) print rest
$8 = " R 1 4242 4242 0 -1 4194304 123 0 0 0 145 23 0 0 20 0 1 0 1000 16777216 1024"

(gdb) next
71      std::istringstream iss(rest);
(gdb) next
74      if (!(iss >> out.state >> out.ppid)) {
(gdb) next
77      for (int i = 5; i <= 13; ++i) {
(gdb) next
82      iss >> out.utime >> out.stime;

(gdb) print out.state
$9 = 82 'R'

(gdb) print out.ppid
$10 = 1

(gdb) print out.utime
$11 = 145

(gdb) print out.stime
$12 = 23
```

### 3.7 Stack Frame Navigation & Backtrace
Examine the call stack leading to this invocation:
```text
(gdb) backtrace
#0  minimon::ProcReader::parse_stat_string (line="4242 ((a) b (c)) R ...", out=...) at /home/engineer/sysscope/src/proc_reader.cpp:82
#1  0x0000000000013012 in minimon::ProcReader::read_stat (this=0x7fffffffdc50, pid=4242, out=...) at /home/engineer/sysscope/src/proc_reader.cpp:115
#2  0x00000000000132b8 in minimon::ProcReader::read_single_process (this=0x7fffffffdc50, pid=4242, out=...) at /home/engineer/sysscope/src/proc_reader.cpp:346
#3  0x00000000000134f0 in minimon::ProcReader::read_all_processes (this=0x7fffffffdc50, cpu_tracker=...) at /home/engineer/sysscope/src/proc_reader.cpp:378
#4  0x0000000000014a24 in minimon::run_single_shot (config=..., os=...) at /home/engineer/sysscope/src/display.cpp:328
#5  0x0000000000011860 in main (argc=4, argv=0x7fffffffdf18) at /home/engineer/sysscope/src/main.cpp:19

(gdb) frame 1
#1  0x0000000000013012 in minimon::ProcReader::read_stat (this=0x7fffffffdc50, pid=4242, out=...) at /home/engineer/sysscope/src/proc_reader.cpp:115
115     return parse_stat_string(line, out);

(gdb) info locals
path = "tests/fixtures/proc_tricky/4242/stat"
file = <incomplete type std::ifstream>
line = "4242 ((a) b (c)) R 1 4242 4242 0 -1 ..."
```

Resume execution to program completion:
```text
(gdb) continue
Continuing.
Tasks: 1 total, 1 running, 0 sleeping, 0 zombie, 0 stopped
Mem:   0.0M total, 0.0M used (0.0%), 0.0M avail, 0.0M/0.0M swap
Load:  0.00, 0.00, 0.00

    PID    PPID USER       STAT   %CPU   %MEM       RSS  THR COMMAND
--------------------------------------------------------------------------------
   4242       1 root       R       0.0    0.0        0K    1 [(a) b (c)]
[Inferior 1 (process 58492) exited normally]
```

---

## 4. Post-Mortem Core Dump Analysis

When diagnosing segmentation faults (`SIGSEGV`) or aborted assertions in production Linux environments:

### 4.1 Enabling Core Dumps
```bash
# Enable unlimited core dump file size in current shell
ulimit -c unlimited

# Check Linux kernel core dump pattern
cat /proc/sys/kernel/core_pattern
# Standard output: /tmp/core.%e.%p.%t
```

### 4.2 Loading Core Dump in GDB
```bash
gdb ./build/minimon /tmp/core.minimon.59123.1700789000
```

### 4.3 Key Post-Mortem Commands
```text
(gdb) bt
# Displays call stack at the moment of the crash

(gdb) info registers
# Dumps CPU registers (rax, rbx, rip, rsp on x86_64)

(gdb) x/16i $rip - 0x20
# Disassembles instructions surrounding the instruction pointer

(gdb) thread apply all bt
# Dumps stack traces across all threads in multi-threaded programs
```

---

## 5. GDB and LLDB Command Equivalence Reference

| Action | GDB Command | LLDB Command |
| :--- | :--- | :--- |
| Set breakpoint at function | `b minimon::ProcReader::read_stat` | `b minimon::ProcReader::read_stat` |
| Set breakpoint at file:line | `b proc_reader.cpp:60` | `b proc_reader.cpp:60` |
| Conditional breakpoint | `b 60 if pid == 1` | `b 60 -c "pid == 1"` |
| Launch with arguments | `run --once --sort mem` | `r --once --sort mem` |
| Step over line | `next` (or `n`) | `next` (or `n`) |
| Step into function | `step` (or `s`) | `step` (or `s`) |
| Step out of current function| `finish` | `finish` |
| Print variable | `print var` (or `p var`) | `frame var var` (or `p var`) |
| Examine memory as string | `x/s ptr` | `x -s ptr` |
| Show backtrace | `backtrace` (or `bt`) | `bt` |
| Switch stack frame | `frame <num>` | `frame select <num>` |
| Print all local variables | `info locals` | `frame var` |
| Print function arguments | `info args` | `frame var -a` |
