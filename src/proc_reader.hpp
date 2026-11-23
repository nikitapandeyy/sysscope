#pragma once

#include <string>
#include <vector>
#include <sys/types.h>

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

private:
    std::string proc_root_;
};

} // namespace minimon
