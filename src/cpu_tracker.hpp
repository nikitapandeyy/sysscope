#pragma once

#include <unordered_map>
#include <vector>
#include <chrono>
#include <sys/types.h>
#include "process.hpp"

namespace minimon {

// Stores historical snapshot data for a process to compute deltas.
struct ProcessCpuSample {
    unsigned long total_ticks{0};     // utime + stime
    unsigned long long starttime{0};  // starttime in ticks after boot (for PID reuse)
    std::chrono::steady_clock::time_point timestamp{};
};

// CpuTracker calculates per-process CPU% using two-sample delta math.
// Handles new processes, terminated processes, PID reuse, and clock jitter.
class CpuTracker {
public:
    explicit CpuTracker(long clk_tck = 0);

    // Update process with its calculated CPU% based on its previous sample.
    // Stores current values for subsequent delta calculation.
    // If the process is seen for the first time or PID reuse is detected,
    // sets cpu_percent = 0.0.
    void update_process_cpu(Process& proc, std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now());

    // Pure delta calculation with explicit numbers (useful for unit tests and simulations).
    [[nodiscard]] double calculate_cpu_percent(
        unsigned long prev_ticks,
        unsigned long curr_ticks,
        unsigned long long prev_starttime,
        unsigned long long curr_starttime,
        double elapsed_seconds
    ) const noexcept;

    // Remove any PIDs from the internal cache that are not present in active_pids,
    // preventing memory leaks as processes exit over time.
    void prune_exited(const std::vector<pid_t>& active_pids);

    // Return the number of tracked PIDs in memory.
    [[nodiscard]] size_t tracked_count() const noexcept {
        return history_.size();
    }

    // Return the system clock ticks per second in use.
    [[nodiscard]] long clk_tck() const noexcept {
        return clk_tck_;
    }

    // Clear all historical samples.
    void clear() noexcept {
        history_.clear();
    }

private:
    long clk_tck_{100};
    std::unordered_map<pid_t, ProcessCpuSample> history_;
};

} // namespace minimon
