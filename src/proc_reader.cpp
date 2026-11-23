#include "proc_reader.hpp"
#include <filesystem>
#include <algorithm>
#include <cctype>

namespace minimon {

ProcReader::ProcReader(std::string proc_root)
    : proc_root_(std::move(proc_root)) {}

std::vector<pid_t> ProcReader::get_pids() const {
    std::vector<pid_t> pids;
    std::error_code ec;

    // Open directory iterator with error code to prevent throwing on missing or unreadable directories
    std::filesystem::directory_iterator it(proc_root_, std::filesystem::directory_options::skip_permission_denied, ec);
    if (ec) {
        return pids;
    }

    for (const auto& entry : it) {
        std::error_code status_ec;
        // In Linux /proc, every process has a directory named after its numeric PID
        if (!entry.is_directory(status_ec)) {
            continue;
        }

        const std::string filename = entry.path().filename().string();
        if (filename.empty()) {
            continue;
        }

        // Verify that the directory name consists solely of digits '0'-'9'
        bool is_numeric = std::all_of(filename.begin(), filename.end(), [](unsigned char c) {
            return std::isdigit(c) != 0;
        });

        if (is_numeric) {
            try {
                auto pid = static_cast<pid_t>(std::stol(filename));
                if (pid > 0) {
                    pids.push_back(pid);
                }
            } catch (...) {
                // Safely skip any value out of pid_t range
            }
        }
    }

    std::sort(pids.begin(), pids.end());
    return pids;
}

} // namespace minimon
