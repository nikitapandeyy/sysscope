# Devlog T3.3: Terminal Table Renderer & System Summary Banner

## Task
Implement the terminal table renderer and system summary banner in `src/display.hpp` and `src/display.cpp`, featuring aligned fixed-width columns, dynamic terminal width detection via `ioctl(TIOCGWINSZ)`, intelligent command line truncation, human-readable memory formatting, and task breakdown metrics.

## What I did
- Authored `src/display.hpp` and `src/display.cpp`:
  - `struct SystemSummary`: tracks task counts categorized by state (`R`, `S`/`D`/`I`, `Z`, `T`), memory totals, and 1m/5m/15m system load averages.
  - `get_terminal_width()`: queries `ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws)` with fallback to the `COLUMNS` environment variable or standard 100-column default.
  - `get_system_loadavg()`: retrieves load averages using POSIX `getloadavg()`.
  - `truncate_string()`: trims long strings cleanly with `...` ellipsis.
  - `format_memory()`: converts raw kilobytes into human-readable `K`, `M`, or `G` suffixes.
  - `render_table()`: formats the summary header banner, column headers (`PID`, `PPID`, `USER`, `STAT`, `%CPU`, `%MEM`, `RSS`, `THR`, `COMMAND`), divider line, and process rows.
- Updated `CMakeLists.txt` to add `src/display.cpp` to `minimon_core`.
- Authored 5 unit tests in `tests/test_display.cpp` testing string truncation, memory formatting, summary statistics aggregation, table rendering, and row limit truncation (`max_rows`).

## Why I did it this way
- In terminal monitoring tools, output readability is critical. By dynamically calculating the width available for the `COMMAND` column (`term_width - fixed_columns_width`), the output never wraps awkwardly on standard terminal emulators, maintaining clean tabular vertical alignment.
- Formatting memory with unit suffixes (`M`, `G`) provides instant comprehension compared to raw 7-digit kilobyte integers.
- Separating summary calculations (`make_summary`) from rendering (`render_table`) allows the same data structures to be rendered as ASCII tables or exported as structured JSON.
- Alternatives considered: External libraries like `ncurses` or `ftxui`. Rejected because `minimon` is designed to be minimal, transparent, and educational with zero external GUI/TUI runtime dependencies.

## Problems encountered
None. All 52 tests passed cleanly on the first run.

## How I solved them
N/A (no problems encountered).

## Linux concepts involved
- Terminal geometry discovery: POSIX `ioctl(2)` using `TIOCGWINSZ` on `STDOUT_FILENO` with `struct winsize`.
- System load averages: `getloadavg(3)` returning the 1, 5, and 15 minute exponential moving averages of runnable and uninterruptible tasks in the Linux scheduler run queue.
- Human-readable memory units (powers of 1024).

## How to verify
```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

## Open questions
- None. Next task is T3.4: CLI argument parsing with all specified flags and `--help`.
