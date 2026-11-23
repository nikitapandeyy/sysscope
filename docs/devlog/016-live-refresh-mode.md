# Devlog T3.5: Live Refresh Mode & Signal Handling

## Task
Implement the interactive live monitoring loop and one-shot runner in `src/display.cpp` and `src/main.cpp`, featuring ANSI cursor control, flicker-free redraws, responsive sleep intervals, and clean `Ctrl+C` signal handling via POSIX `sigaction`.

## What I did
- Added signal handling and execution coordinator functions in `src/display.hpp` and `src/display.cpp`:
  - `setup_signal_handlers()`: installs POSIX `sigaction` handlers for `SIGINT` and `SIGTERM`.
  - `is_stop_requested()` and `request_stop()`: thread-safe atomic controls via `std::atomic<bool> g_stop_requested`.
  - `struct TerminalGuard`: RAII guard that hides the terminal cursor on entry (`\033[?25l`) and restores cursor visibility and default color attributes on exit (`\033[?25h\033[0m\n`).
  - `run_single_shot()`: performs a two-sample delta pass, filters, sorts, reads system memory, and prints the process table once.
  - `run_live_loop()`: periodically reads system telemetry, moves cursor to home position (`\033[H`) to prevent full-screen redraw flickering, and uses a responsive 50ms stepped sleep loop so `SIGINT` (`Ctrl+C`) terminates immediately without waiting for the full interval to expire.
- Updated `src/main.cpp` to integrate argument parsing, dispatching to `run_single_shot()` or `run_live_loop()`.
- Authored 3 unit tests in `tests/test_display.cpp` verifying signal registration, single-shot execution against fixtures, and live loop termination upon stop request.

## Why I did it this way
- Repositioning the cursor to the home coordinate `\033[H` rather than clearing the full display (`\033[2J`) eliminates screen flicker between refresh frames in terminal emulators.
- Using POSIX `sigaction` with an `std::atomic<bool>` flag ensures async-signal-safety, preventing deadlocks or undefined behavior in signal handlers.
- Stepping the sleep loop in 50ms increments ensures that if a user sets `--interval 5`, pressing `Ctrl+C` exits within 50ms rather than hanging for up to 5 seconds.
- Alternatives considered: Calling `system("clear")`. Rejected because spawning a subprocess every second is inefficient, slow, and causes severe visual stutter.

## Problems encountered
- `DisplayLiveTest.RunSingleShotOnFixture` failed on initial run looking for `"systemd"`.
  - *Symptom*: Expected `"systemd"` in output stream, but test failed.
  - *Root Cause*: PID 1 in the fixture has executable name `systemd`, but its `cmdline` is `/sbin/init`. In `render_table()`, the `COMMAND` column prints the command line (`/sbin/init`), not the short executable name.
- Compiler warnings `-Wunused-result` on nodiscard reader methods in `src/display.cpp`.
  - *Symptom*: Warnings on ignoring return values of `read_all_processes` and `read_meminfo`.
  - *Root Cause*: Initial sampling passes intentionally discard the intermediate vectors since their sole purpose is populating `CpuTracker` baselines.

## How I solved them
- Updated the test assertion to check for `/sbin/init`, `/bin/bash`, and `[kthreadd]`.
- Explicitly cast unused calls to `static_cast<void>`.

## Linux concepts involved
- POSIX signal handling: `sigaction(2)`, async-signal-safety, and graceful termination on `SIGINT` / `SIGTERM`.
- ANSI X3.64 / VT100 escape sequences: cursor positioning (`\033[H`), cursor visibility (`\033[?25l` / `\033[?25h`).
- Terminal detection: `isatty(STDOUT_FILENO)` verifying whether stdout is an interactive TTY or redirected to a pipe/file.

## How to verify
```bash
cmake --build build
ctest --test-dir build --output-on-failure
./build/minimon --once --proc-root tests/fixtures/proc_full
```

## Open questions
- None. Next task is T3.6: `--json` output mode and JSON schema documentation.
