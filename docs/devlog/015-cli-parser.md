# Devlog T3.4: CLI Argument Parsing & Help Documentation

## Task
Implement command-line interface argument parsing in `src/cli.hpp` and `src/cli.cpp` supporting all required user options (`--search`, `--sort`, `--top`, `--min-cpu`, `--min-mem`, `--user`, `--state`, `--pid`, `--once`, `--interval`, `--json`, and `--help`) with robust validation and descriptive usage documentation.

## What I did
- Authored `src/cli.hpp` declaring:
  - `struct CliConfig`: encapsulates CLI switches, thresholds, intervals, sort selections, and error messages.
  - `parse_cli_args(int argc, const char* const argv[], CliConfig& config)`: command-line argument parser.
  - `print_help(std::ostream& os)`: formats complete help text and examples.
- Authored `src/cli.cpp` implementing:
  - Iteration over `argv` with bounds checking for option arguments.
  - Type-safe conversions (`std::stoi`, `std::stod`, `std::stoul`) wrapped in exception guards returning descriptive errors upon invalid numeric inputs.
  - Comprehensive flags: `--help` (`-h`), `--once`, `--json`, `--search` (`-s`), `--user` (`-u`), `--state`, `--pid` (`-p`), `--min-cpu`, `--min-mem`, `--sort`, `--asc`, `--desc`, `--top` (`-n`), `--interval` (`-i`), and `--proc-root`.
- Updated `CMakeLists.txt` to include `src/cli.cpp` in `minimon_core`.
- Authored 5 unit tests in `tests/test_cli.cpp` testing default configuration, help flag, search/filter flags, sort/display options, and missing/invalid arguments.

## Why I did it this way
- Building a lightweight argument parser directly in standard C++ eliminates external dependencies like `getopt_long` (which has platform variations between GNU and BSD/macOS) or heavyweight libraries like `boost::program_options`.
- Isolating argument parsing in `src/cli.hpp` instead of burying it inside `main.cpp` allows the parsing logic and error handling to be directly unit-tested with GoogleTest.
- Alternatives considered: Using `getopt_long(3)`. Rejected because it mutates global state (`optarg`, `optind`), is not thread-safe, and does not support testing without resetting C runtime globals.

## Problems encountered
None. All 57 tests passed on the first run.

## How I solved them
N/A (no problems encountered).

## Linux concepts involved
- POSIX CLI conventions: handling short flags (`-h`, `-s`), long options (`--search`, `--interval`), and numeric argument conversion.
- Argument boundary safety: preventing buffer overruns when options lack required values.

## How to verify
```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

## Open questions
- None. Next task is T3.5: live refresh mode (ANSI screen clearing, configurable interval, clean `Ctrl+C` signal handling restoring the terminal).
