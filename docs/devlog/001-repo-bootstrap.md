# Devlog T1.1: Repo Bootstrap

## Task
Initialize the `minimon` repository layout, configure the root CMake build system with C++17 and strict compiler warnings, establish the directory structure, and set up documentation and devlog tracking on branch `phase-1-learn-proc`.

## What I did
- Switched to the `phase-1-learn-proc` branch from `main`.
- Created the project directory layout:
  - `src/`: for core monitor implementation (`proc_reader`, `process`, `cpu_tracker`, `filter`, `display`, `main.cpp`).
  - `tests/`: for unit tests and `tests/fixtures/` for synthetic `/proc` test data.
  - `experiments/`: for exploratory learning programs.
  - `docs/devlog/`: for discrete task logs and index table.
  - `docs/learning/`: for Linux and `/proc` filesystem conceptual guides.
- Authored the root `CMakeLists.txt` targeting C++17 with `-Wall -Wextra -Wpedantic`.
- Created `src/main.cpp` with a minimal entry point.
- Created `README.md` skeleton outlining features, structure, and usage.
- Created `docs/devlog/README.md` index and this task devlog.

## Why I did it this way
- Kept the code and build system minimal and clean without unnecessary abstractions or complex target hierarchies.
- Configured `-Wall -Wextra -Wpedantic` from the start so that compiler warnings catch common systems programming pitfalls (unused variables, sign mismatches, unhandled cases) immediately.
- Alternatives considered: Multi-library CMake targets. Rejected because `minimon` is intended to be concise, easy to read, and educational without excess build bureaucracy.

## Problems encountered
None. Directory creation, CMake configuration, and main stub setup completed cleanly.

## How I solved them
N/A (no problems encountered).

## Linux concepts involved
- Build automation and C++17 toolchain setup for Linux systems programming.
- Compiler warning hygiene (`-Wall -Wextra -Wpedantic`) to safeguard low-level POSIX and string operations.

## How to verify
```bash
cmake -B build -S .
cmake --build build
./build/minimon
```

## Open questions
- None for this bootstrap task. In T1.2, document the `/proc` filesystem details and capture sample outputs.
