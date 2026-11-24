#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <sys/types.h>
#include "process.hpp"

namespace minimon {

// System-wide memory statistics extracted from /proc/meminfo.
struct SystemMemory {
    uint64_t total_kb{0};
    uint64_t free_kb{0};
    uint64_t available_kb{0};
    uint64_t buffers_kb{0};
    uint64_t cached_kb{0};
    uint64_t swap_total_kb{0};
    uint64_t swap_free_kb{0};

    [[nodiscard]] uint64_t used_kb() const noexcept {
        if (available_kb > 0 && total_kb >= available_kb) {
            return total_kb - available_kb;
        }
        uint64_t non_used = free_kb + buffers_kb + cached_kb;
        return total_kb >= non_used ? total_kb - non_used : 0;
    }

    [[nodiscard]] double used_percent() const noexcept {
        return total_kb > 0 ? (static_cast<double>(used_kb()) / static_cast<double>(total_kb)) * 100.0 : 0.0;
    }

    [[nodiscard]] double total_mb() const noexcept {
        return static_cast<double>(total_kb) / 1024.0;
    }

    [[nodiscard]] double used_mb() const noexcept {
        return static_cast<double>(used_kb()) / 1024.0;
    }
};

// System-wide aggregate CPU times in jiffies extracted from the first line of /proc/stat.
struct SystemCpuTimes {
    unsigned long long user{0};
    unsigned long long nice{0};
    unsigned long long system{0};
    unsigned long long idle{0};
    unsigned long long iowait{0};
    unsigned long long irq{0};
    unsigned long long softirq{0};
    unsigned long long steal{0};
    unsigned long long guest{0};
    unsigned long long guest_nice{0};

    [[nodiscard]] unsigned long long total_time() const noexcept {
        return user + nice + system + idle + iowait + irq + softirq + steal;
    }

    [[nodiscard]] unsigned long long idle_time() const noexcept {
        return idle + iowait;
    }

    [[nodiscard]] unsigned long long active_time() const noexcept {
        return total_time() >= idle_time() ? total_time() - idle_time() : 0;
    }
};

// ProcReader parses Linux /proc virtual filesystem files.
// It accepts a configurable root path (default "/proc") so that unit and
// integration tests can inject mock directory fixtures.
class ProcReader {
public:
    explicit ProcReader(std::string proc_root = "/proc");

    // Return the proc root path currently in use.
    [[nodiscard]] const std::string& root() const noexcept {
        return proc_root_;
    }

    // Enumerate all active Process IDs by listing all purely numeric
    // subdirectories within the proc root directory.
    [[nodiscard]] std::vector<pid_t> get_pids() const;

    // Resolve a numeric UID to a username string (e.g. 0 -> "root").
    // Uses POSIX getpwuid_r with an in-memory cache and a numeric fallback.
    [[nodiscard]] std::string get_username(uid_t uid) const;

    // Clear the username cache.
    void clear_user_cache() const;

    // Read and parse /proc/[pid]/stat into the provided Process object.
    // Returns true on success; returns false if the file cannot be opened
    // (e.g. process terminated: ENOENT) or is malformed.
    [[nodiscard]] bool read_stat(pid_t pid, Process& out) const;

    // Parse a raw single-line /proc/[pid]/stat text string.
    // Handles tricky comm names containing spaces and nested parentheses.
    static bool parse_stat_string(const std::string& line, Process& out);

    // Read and parse /proc/[pid]/status into the provided Process object.
    // Extracts: Name, State, Uid, VmSize, VmRSS, Threads.
    [[nodiscard]] bool read_status(pid_t pid, Process& out) const;

    // Parse status key-value lines from an input stream.
    static bool parse_status_stream(std::istream& is, Process& out);

    // Read and parse /proc/[pid]/cmdline into out.cmdline.
    // Converts internal NUL separators to spaces.
    [[nodiscard]] bool read_cmdline(pid_t pid, Process& out) const;

    // Parse raw NUL-separated cmdline buffer into space-separated string.
    static std::string parse_cmdline_buffer(const char* buf, size_t length);

    // Read and parse /proc/meminfo for system memory statistics.
    [[nodiscard]] bool read_meminfo(SystemMemory& out) const;

    // Parse /proc/meminfo content from stream.
    static bool parse_meminfo_stream(std::istream& is, SystemMemory& out);

    // Read and parse aggregate CPU times from /proc/stat.
    [[nodiscard]] bool read_system_cpu(SystemCpuTimes& out) const;

    // Parse aggregate CPU times from /proc/stat stream.
    static bool parse_stat_cpu_stream(std::istream& is, SystemCpuTimes& out);

    // Read full telemetry for a single process (stat, status, cmdline, username).
    // Returns true on success; false if process does not exist or terminated.
    [[nodiscard]] bool read_single_process(pid_t pid, Process& out) const;

    // Read full telemetry for all active processes in the proc root.
    // Gracefully skips any process that vanishes mid-read (e.g. ENOENT).
    // Computes memory percentage against /proc/meminfo and updates CPU%
    // using the provided CpuTracker. Prunes exited processes from the tracker.
    [[nodiscard]] std::vector<Process> read_all_processes(class CpuTracker& cpu_tracker) const;

private:
    std::string proc_root_;
    mutable std::unordered_map<uid_t, std::string> user_cache_;
};

} // namespace minimon
