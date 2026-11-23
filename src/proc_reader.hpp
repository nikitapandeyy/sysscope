#pragma once

#include <string>
#include <vector>
#include <sys/types.h>
#include "process.hpp"

namespace minimon {

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

private:
    std::string proc_root_;
};

} // namespace minimon
