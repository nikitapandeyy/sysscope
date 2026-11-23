#pragma once

#include <string>
#include <sys/types.h>
#include <cstdint>

namespace minimon {

// Represents telemetry captured for a single Linux process.
struct Process {
    pid_t pid{0};                     // Process ID (TGID)
    pid_t ppid{0};                    // Parent Process ID
    std::string name;                 // Executable name (comm)
    char state{'?'};                  // Execution state (R, S, D, Z, T, etc.)
    std::string user{"unknown"};      // Owner username resolved from UID
    uid_t uid{0};                     // Real User ID
    double cpu_percent{0.0};          // Calculated CPU utilization percentage
    uint64_t rss_kb{0};               // Resident Set Size in kilobytes
    uint64_t vm_size_kb{0};           // Virtual Memory Size in kilobytes
    double mem_percent{0.0};          // RSS as a percentage of total physical RAM
    long threads{1};                  // Number of threads in the process
    unsigned long long starttime{0};  // Process start time in ticks after boot (for PID reuse)
    std::string cmdline;              // Full command line arguments
    unsigned long utime{0};           // User CPU time in clock ticks
    unsigned long stime{0};           // System (kernel) CPU time in clock ticks

    // Helper to get RSS formatted in megabytes
    [[nodiscard]] double rss_mb() const noexcept {
        return static_cast<double>(rss_kb) / 1024.0;
    }
};

} // namespace minimon
