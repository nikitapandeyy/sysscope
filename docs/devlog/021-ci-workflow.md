# Devlog Task 021: GitHub Actions Continuous Integration Workflow

## Task
Implement automated multi-compiler Continuous Integration (CI) in `.github/workflows/ci.yml` compiling with GCC and Clang on Ubuntu and AppleClang on macOS, running all tests, executing dynamic sanitizers, checking memory leaks with Valgrind, and exercising live Linux `/proc` execution.

---

## What I Did
1. **GitHub Actions Matrix (`.github/workflows/ci.yml`)**:
   - Configured cross-platform matrix testing across:
     - Ubuntu 22.04 with GCC 11 (Release mode with Valgrind memory leak verification).
     - Ubuntu 22.04 with Clang 14 (Release mode).
     - Ubuntu 22.04 with Clang 14 (Debug mode with AddressSanitizer and UndefinedBehaviorSanitizer enabled).
     - macOS 13 with AppleClang (Release mode).
2. **Build and Verification Steps**:
   - Automated checkout and dependency installation (`cmake`, `build-essential`, `valgrind`).
   - CMake configuration and parallel compilation across all available CPU cores (`nproc`).
   - Automated execution of GoogleTest test suite via `ctest --output-on-failure`.
   - Automated Valgrind check ensuring zero memory leaks with `--error-exitcode=1`.
   - Verification of live host execution on Linux running `./build/minimon --once` and `./build/minimon --json` against real kernel `/proc`.

---

## Why I Did It This Way
- **Multi-Compiler Matrix**: Compiling against both GCC and Clang ensures strict ISO C++17 compliance and catches compiler-specific diagnostics or non-portable idioms early.
- **Dedicated Sanitizer Job**: Running an explicit ASan + UBSan pipeline in CI guarantees that memory regressions, buffer overruns, or undefined behavior cannot be merged into `main`.
- **Valgrind on Linux**: Executes native Valgrind memory checks in the Linux Ubuntu runner where Linux ELF binary instrumentation and syscall emulation are natively supported.

---

## Problems Encountered
- **Problem**: Python `yaml` module was not installed in the local environment when attempting to validate workflow syntax.
  - **Symptom**: `ModuleNotFoundError: No module named 'yaml'`.
  - **Root Cause**: `PyYAML` package not bundled in system Python3.
  - **Solution**: Validated `.github/workflows/ci.yml` using system Ruby's built-in YAML parser (`ruby -ryaml -e "YAML.load_file(...)`), confirming standard YAML structure.

---

## Linux/OS Concepts Involved
- **Continuous Integration for Systems Software**: Kernel interface programs relying on `/proc` require testing in actual Linux kernels to validate real `/proc/meminfo`, `/proc/stat`, and `/proc/[pid]/...` generation alongside synthetic test fixtures.
- **Valgrind in CI Pipelines**: Setting `--error-exitcode=1` transforms Valgrind from an advisory diagnostic tool into a hard gatekeeper preventing memory leaks in production codebases.

---

## How to Verify
```bash
# 1. Validate YAML syntax
ruby -ryaml -e "YAML.load_file('.github/workflows/ci.yml')"

# 2. Check workflow file existence and formatting
cat .github/workflows/ci.yml
```

---

## Open Questions / Follow-ups
- Proceed to Task T4.5: Final project polish, comprehensive top-level `README.md` documentation, and release tagging `v1.0.0`.
