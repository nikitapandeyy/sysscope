# Devlog Task 019: AddressSanitizer, UndefinedBehaviorSanitizer, and Quality Audit

## Task
Integrate AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan) into the CMake build system, verify all unit and integration tests under dynamic instrumentation with zero memory leaks or undefined behaviors, and author `docs/quality.md`.

---

## What I Did
1. **Sanitizer Build Option (`CMakeLists.txt`)**:
   - Added `ENABLE_SANITIZERS` CMake option enabling `-fsanitize=address,undefined -fno-omit-frame-pointer -g` for compilation and linking when building with Clang or GCC.
2. **Dynamic Instrumentation Verification**:
   - Rebuilt the entire codebase including `minimon_core`, `minimon` CLI, and all 67 test suites with ASan and UBSan active.
   - Executed the full test suite and manual CLI runs (`--once` and `--json`) with zero runtime errors, zero memory leaks, and zero undefined behavior warnings.
3. **Quality & Memory Safety Documentation (`docs/quality.md`)**:
   - Authored comprehensive documentation explaining ASan, UBSan, and memory audit results.
   - Documented why Valgrind does not support macOS Darwin on Apple Silicon ARM64, and provided explicit Valgrind flags and execution commands for Linux CI environments.
   - Summarized defensive C++17 memory hygiene rules practiced in `minimon`.

---

## Why I Did It This Way
- **Option Flag (`ENABLE_SANITIZERS`)**: Leaving sanitizers optional in CMake allows release builds to compile with maximum optimizations (`-O3`) without instrumentation overhead, while giving developers and CI workflows a single switch (`-DENABLE_SANITIZERS=ON`) to perform rigorous sanitization.
- **Combined ASan + UBSan**: Combining address sanitization with undefined behavior sanitization catches both memory safety bugs (out-of-bounds, use-after-free, leaks) and subtle language infractions (signed overflow, null dereferences, misaligned accesses) simultaneously.
- **Contextualizing Valgrind on Darwin**: Rather than ignoring Valgrind on macOS, the documentation explains the architectural limitations (lack of an ARM64 JIT engine in Valgrind upstream) and establishes how Valgrind is run in Linux environments.

---

## Problems Encountered
- **Problem**: When configuring CMake in an alternate directory (`build_san`), CMake's FetchContent attempted to download GoogleTest again from GitHub, failing because the default sandbox restricts external network access.
  - **Symptom**: `status_string: "Couldn't resolve host name", log: Could not resolve host: github.com`.
  - **Root Cause**: FetchContent was invoked in a clean directory where `_deps/googletest-src` was not yet populated.
  - **Solution**: Removed `build_san` and configured the existing `build` directory directly (`cmake -B build -DENABLE_SANITIZERS=ON`), reusing the existing local GoogleTest source cache.

---

## Linux/OS Concepts Involved
- **Shadow Memory (AddressSanitizer)**: ASan maps 1/8th of the virtual address space to shadow memory. Each 8-byte application chunk has a corresponding 1-byte shadow tag indicating whether the memory is addressable or poisoned. Redzones are inserted around buffers, and free lists quarantine deallocated blocks to immediately catch use-after-free.
- **Dynamic Binary Instrumentation (Valgrind)**: Valgrind translates machine code into VEX intermediate representation and instruments memory operations at runtime. On Linux x86_64 / arm64, it tracks valid address bits (A-bits) and valid value bits (V-bits) to catch uninitialized reads and invalid memory accesses.

---

## How to Verify
```bash
# 1. Configure and build with sanitizers enabled
cmake -B build -DENABLE_SANITIZERS=ON
cmake --build build

# 2. Run all tests under ASan and UBSan
ctest --test-dir build --output-on-failure

# 3. Verify standalone CLI runs cleanly
./build/minimon --proc-root tests/fixtures/proc_full --once
./build/minimon --proc-root tests/fixtures/proc_full --json
```

---

## Open Questions / Follow-ups
- Proceed to Task T4.3: Conduct and document a realistic GDB debugging session in `docs/gdb-session.md`.
