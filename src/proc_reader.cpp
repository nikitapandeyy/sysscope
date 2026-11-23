#include "proc_reader.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
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

bool ProcReader::parse_stat_string(const std::string& line, Process& out) {
    // 1. The comm field is enclosed in parentheses: (comm)
    // Because comm can contain spaces and parentheses (e.g. "(a b)", "((a) b (c))"),
    // we locate the first '(' and the LAST ')' in the line.
    size_t first_paren = line.find('(');
    size_t last_paren = line.rfind(')');

    if (first_paren == std::string::npos || last_paren == std::string::npos || last_paren <= first_paren) {
        return false;
    }

    // 2. Extract PID (before first '(')
    try {
        std::string pid_str = line.substr(0, first_paren);
        out.pid = static_cast<pid_t>(std::stol(pid_str));
    } catch (...) {
        return false;
    }

    // 3. Extract comm (between first '(' and last ')')
    out.name = line.substr(first_paren + 1, last_paren - (first_paren + 1));

    // 4. Everything after last_paren + 1 consists of space-separated positional tokens
    // starting with field 3 (state)
    std::string rest = line.substr(last_paren + 1);
    std::istringstream iss(rest);

    // Field 3: state (%c)
    if (!(iss >> out.state)) return false;

    // Field 4: ppid (%d)
    long ppid_val = 0;
    if (!(iss >> ppid_val)) return false;
    out.ppid = static_cast<pid_t>(ppid_val);

    // Fields 5 through 13:
    // 5: pgrp, 6: session, 7: tty_nr, 8: tpgid, 9: flags,
    // 10: minflt, 11: cminflt, 12: majflt, 13: cmajflt
    std::string dummy;
    for (int i = 5; i <= 13; ++i) {
        if (!(iss >> dummy)) return false;
    }

    // Field 14: utime (%lu)
    if (!(iss >> out.utime)) return false;

    // Field 15: stime (%lu)
    if (!(iss >> out.stime)) return false;

    // Fields 16 through 19:
    // 16: cutime, 17: cstime, 18: priority, 19: nice
    for (int i = 16; i <= 19; ++i) {
        if (!(iss >> dummy)) return false;
    }

    // Field 20: num_threads (%ld)
    if (!(iss >> out.threads)) return false;

    // Field 21: itrealvalue
    if (!(iss >> dummy)) return false;

    // Field 22: starttime (%llu)
    if (!(iss >> out.starttime)) return false;

    return true;
}

bool ProcReader::read_stat(pid_t pid, Process& out) const {
    std::string path = proc_root_ + "/" + std::to_string(pid) + "/stat";
    std::ifstream file(path);
    if (!file.is_open()) {
        // Process may have terminated (ENOENT) or lacks permissions (EACCES)
        return false;
    }

    std::string line;
    if (!std::getline(file, line)) {
        return false;
    }

    return parse_stat_string(line, out);
}

bool ProcReader::parse_status_stream(std::istream& is, Process& out) {
    std::string line;
    while (std::getline(is, line)) {
        if (line.rfind("Name:", 0) == 0) {
            // Name:    bash
            size_t pos = line.find_first_not_of(" \t", 5);
            if (pos != std::string::npos && out.name.empty()) {
                out.name = line.substr(pos);
            }
        } else if (line.rfind("State:", 0) == 0) {
            // State:   S (sleeping)
            size_t pos = line.find_first_not_of(" \t", 6);
            if (pos != std::string::npos && !line.empty()) {
                out.state = line[pos];
            }
        } else if (line.rfind("Uid:", 0) == 0) {
            // Uid: 1000    1000    1000    1000
            std::istringstream uid_iss(line.substr(4));
            long real_uid = 0;
            if (uid_iss >> real_uid) {
                out.uid = static_cast<uid_t>(real_uid);
            }
        } else if (line.rfind("VmSize:", 0) == 0) {
            // VmSize:     15808 kB
            std::istringstream sz_iss(line.substr(7));
            uint64_t sz = 0;
            if (sz_iss >> sz) {
                out.vm_size_kb = sz;
            }
        } else if (line.rfind("VmRSS:", 0) == 0) {
            // VmRSS:       4840 kB
            std::istringstream rss_iss(line.substr(6));
            uint64_t rss = 0;
            if (rss_iss >> rss) {
                out.rss_kb = rss;
            }
        } else if (line.rfind("Threads:", 0) == 0) {
            // Threads: 1
            std::istringstream th_iss(line.substr(8));
            long th = 1;
            if (th_iss >> th) {
                out.threads = th;
            }
        }
    }
    return true;
}

bool ProcReader::read_status(pid_t pid, Process& out) const {
    std::string path = proc_root_ + "/" + std::to_string(pid) + "/status";
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }
    return parse_status_stream(file, out);
}

std::string ProcReader::parse_cmdline_buffer(const char* buf, size_t length) {
    if (!buf || length == 0) {
        return "";
    }

    std::string result;
    result.reserve(length);

    for (size_t i = 0; i < length; ++i) {
        char c = buf[i];
        if (c == '\0') {
            // Replace NUL byte with a single space separator
            if (i + 1 < length && !result.empty() && result.back() != ' ') {
                result.push_back(' ');
            }
        } else {
            result.push_back(c);
        }
    }

    // Trim trailing space if any
    while (!result.empty() && result.back() == ' ') {
        result.pop_back();
    }

    return result;
}

bool ProcReader::read_cmdline(pid_t pid, Process& out) const {
    std::string path = proc_root_ + "/" + std::to_string(pid) + "/cmdline";
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        out.cmdline = "";
        return false;
    }

    std::vector<char> buffer(4096);
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    std::streamsize bytes_read = file.gcount();

    if (bytes_read > 0) {
        out.cmdline = parse_cmdline_buffer(buffer.data(), static_cast<size_t>(bytes_read));
    } else {
        out.cmdline = "";
    }

    return true;
}

} // namespace minimon
