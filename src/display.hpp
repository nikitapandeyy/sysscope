#pragma once

#include <iostream>
#include <vector>
#include <string>
#include "process.hpp"
#include "proc_reader.hpp"

namespace minimon {

// Summary metrics displayed above the process table.
struct SystemSummary {
    size_t total_tasks{0};
    size_t running_tasks{0};
    size_t sleeping_tasks{0};
    size_t zombie_tasks{0};
    size_t stopped_tasks{0};
    SystemMemory memory;
    double load_1m{0.0};
    double load_5m{0.0};
    double load_15m{0.0};
};

// Detect the current terminal width in columns (defaults to 100 if piped or unknown).
[[nodiscard]] size_t get_terminal_width() noexcept;

// Read system load averages using POSIX getloadavg().
void get_system_loadavg(double& l1, double& l5, double& l15) noexcept;

// Build a SystemSummary struct from the process list and memory state.
[[nodiscard]] SystemSummary make_summary(
    const std::vector<Process>& procs,
    const SystemMemory& mem
);

// Truncate a string to at most max_len characters, appending "..." if truncated.
[[nodiscard]] std::string truncate_string(std::string_view str, size_t max_len);

// Format memory size in human-readable units (kB, MB, GB).
[[nodiscard]] std::string format_memory(uint64_t kb);

// Render the formatted header banner and process table to an output stream.
void render_table(
    std::ostream& os,
    const std::vector<Process>& procs,
    const SystemSummary& summary,
    size_t max_rows = 0,
    size_t term_width = 0
);

// Install POSIX sigaction handlers for SIGINT and SIGTERM to enable clean shutdown.
void setup_signal_handlers() noexcept;

// Check whether shutdown has been requested (e.g. via Ctrl+C / SIGINT).
[[nodiscard]] bool is_stop_requested() noexcept;

// Request shutdown programmatically.
void request_stop() noexcept;

// Run a single-shot inspection pass and output to os.
void run_single_shot(const struct CliConfig& config, std::ostream& os);

// Run the interactive live monitoring loop updating every config.interval seconds.
// Restores cursor and terminal state cleanly upon exit.
void run_live_loop(const struct CliConfig& config, std::ostream& os);

} // namespace minimon
