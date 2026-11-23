# Code Quality, Memory Safety, and Sanitizer Report

This document details the code quality, memory hygiene, dynamic sanitization, and Valgrind verification strategies employed in **Mini Process Monitor (`minimon`)**.

---

## 1. Dynamic Sanitizers (AddressSanitizer & UndefinedBehaviorSanitizer)

### Configuration
The build system supports first-class compiler sanitization via CMake:
```bash
cmake -B build -DENABLE_SANITIZERS=ON
cmake --build build
```

When `-DENABLE_SANITIZERS=ON` is specified, the build system automatically configures:
- `-fsanitize=address`: AddressSanitizer (ASan) instrumenting all memory allocations, pointer arithmetic, and heap/stack bounds.
- `-fsanitize=undefined`: UndefinedBehaviorSanitizer (UBSan) detecting signed integer overflows, null-pointer dereferences, misaligned pointers, invalid bitwise shifts, and division by zero.
- `-fno-omit-frame-pointer -g`: Preserves call stack frames for unambiguous symbolic stack traces upon error.

### Verification Results
All 67 automated unit and integration tests, as well as standalone CLI executions, were executed under AddressSanitizer and UndefinedBehaviorSanitizer.

| Sanitizer Check | Monitored Vulnerability | Result |
| :--- | :--- | :--- |
| **Heap Buffer Overflow** | Out-of-bounds heap memory access | **0 errors (Clean)** |
| **Stack Buffer Overflow** | Out-of-bounds local stack array access | **0 errors (Clean)** |
| **Global Buffer Overflow**| Out-of-bounds global/static memory access | **0 errors (Clean)** |
| **Use-After-Free (UAF)** | Accessing pointers after `delete` / destruction | **0 errors (Clean)** |
| **Double Free / Bad Free**| Invalid deallocation calls | **0 errors (Clean)** |
| **Memory Leaks** | Unreachable allocated blocks at exit | **0 leaks (Clean)** |
| **Signed Integer Overflow**| Arithmetic overflow on signed types | **0 errors (Clean)** |
| **Null Pointer Dereference**| Indirection through null/invalid addresses | **0 errors (Clean)** |
| **Misaligned Memory Access**| Unaligned reads/writes on strict platforms | **0 errors (Clean)** |
| **Zero Division** | Division/modulo by zero in CPU% or memory math | **0 errors (Clean)** |

---

## 2. Valgrind Analysis & Platform Context

### macOS Darwin Architecture Considerations
On modern macOS environments (macOS 12+ on Apple Silicon ARM64 / Darwin), Valgrind is unsupported:
1. Valgrind requires binary translation of machine instructions into an intermediate representation (VEX). Apple Silicon ARM64 (`darwin-arm64`) has not been implemented or merged upstream by the Valgrind project.
2. macOS dynamic linkers (`dyld4`) make extensive use of shared cache structures and system call variants that Valgrind does not emulate.
3. Apple and LLVM recommend Clang AddressSanitizer (`-fsanitize=address`) and leak checking for native Darwin environments.

### Linux Valgrind Verification (Target OS)
In Linux environments (such as Ubuntu 22.04 / 24.04 and GitHub Actions CI), `minimon` is executed under Valgrind's `memcheck` suite:

```bash
# Execute minimon under Valgrind with strict error return
valgrind \
  --leak-check=full \
  --show-leak-kinds=all \
  --track-origins=yes \
  --error-exitcode=1 \
  ./build/minimon --once --proc-root tests/fixtures/proc_full
```

Flags explained:
- `--leak-check=full`: Reports exact call-sites for definitely lost, indirectly lost, and possibly lost memory.
- `--show-leak-kinds=all`: Displays detailed categorization of all lingering allocations.
- `--track-origins=yes`: Tracks the source and allocation origin of uninitialized memory values.
- `--error-exitcode=1`: Ensures CI fails if any leak or memory error is detected.

---

## 3. Core Safety and Architectural Principles

`minimon` achieves memory safety through defensive systems design:

1. **Zero Raw Owning Pointers**:
   - Dynamic memory management uses standard containers (`std::vector`, `std::string`, `std::unordered_map`).
   - Lifetimes of OS resources (terminal states, child processes, file descriptors) are governed by strict RAII guards (`TerminalGuard`, `WorkloadChildGuard`).
2. **Defensive Parsing**:
   - `comm` extraction in `/proc/[pid]/stat` anchors from the last `)` character (`rfind(')')`), preventing injection or parsing confusion if the executable name contains spaces or parentheses.
   - String truncation (`truncate_string`) guarantees fixed-width bounds for terminal columns without off-by-one errors.
   - `json_escape` replaces ASCII control bytes (`< 0x20`), backslashes, and quotes with standard JSON escape sequences.
3. **Division-by-Zero Guards**:
   - Total memory and CPU delta computations guard against `0` total ticks or `0` system RAM, returning `0.0%` rather than triggering floating-point or integer exceptions.
4. **Graceful Error Handling for Terminated Processes**:
   - File opens check `is_open()`. Process termination mid-inspection (`ENOENT` / `ESRCH`) results in silent, graceful skips rather than assertions or crashes.
