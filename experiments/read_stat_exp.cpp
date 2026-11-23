// experiments/read_stat_exp.cpp
//
// A throwaway investigative C++ program that reads and parses /proc/self/stat
// (or a provided sample string if running on non-Linux systems like macOS).
//
// Demonstrates:
// 1. Reading /proc/self/stat using POSIX file descriptor or ifstream.
// 2. Querying sysconf(_SC_CLK_TCK) for user clock tick frequency.
// 3. Robust parsing of the `comm` field by finding the LAST ')' to handle
//    process names containing spaces and parentheses (e.g. "Web Content", "(sd-pam)").

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unistd.h>
#include <cerrno>
#include <cstring>

struct StatFields {
    int pid{0};
    std::string comm;
    char state{'?'};
    int ppid{0};
    unsigned long utime{0};
    unsigned long stime{0};
    long num_threads{0};
    unsigned long long starttime{0};
};

bool parse_stat_line(const std::string& line, StatFields& out) {
    // 1. The comm field is enclosed in parentheses: (comm)
    // Because comm can contain spaces and parentheses, we locate the first '('
    // and the LAST ')' in the line.
    size_t first_paren = line.find('(');
    size_t last_paren = line.rfind(')');

    if (first_paren == std::string::npos || last_paren == std::string::npos || last_paren <= first_paren) {
        std::cerr << "Error: Malformed stat line (could not locate '(' and ')')\n";
        return false;
    }

    // 2. Extract PID (before first '(')
    try {
        out.pid = std::stoi(line.substr(0, first_paren));
    } catch (const std::exception& e) {
        std::cerr << "Error parsing PID: " << e.what() << "\n";
        return false;
    }

    // 3. Extract comm (between first '(' and last ')')
    out.comm = line.substr(first_paren + 1, last_paren - (first_paren + 1));

    // 4. Everything after last_paren + 1 consists of space-separated tokens starting with field 3 (state)
    std::string rest = line.substr(last_paren + 1);
    std::istringstream iss(rest);

    // Field 3: state (%c)
    if (!(iss >> out.state)) return false;

    // Field 4: ppid (%d)
    if (!(iss >> out.ppid)) return false;

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
    if (!(iss >> out.num_threads)) return false;

    // Field 21: itrealvalue
    if (!(iss >> dummy)) return false;

    // Field 22: starttime (%llu)
    if (!(iss >> out.starttime)) return false;

    return true;
}

int main(int argc, char* argv[]) {
    std::cout << "=== /proc/self/stat Parser Experiment ===\n";

    long clk_tck = sysconf(_SC_CLK_TCK);
    std::cout << "System clock ticks per second (_SC_CLK_TCK): " << clk_tck << "\n\n";

    std::string stat_content;

    // Check if user passed a sample string or file via CLI
    if (argc > 1) {
        std::cout << "Using input passed via argument:\n" << argv[1] << "\n\n";
        stat_content = argv[1];
    } else {
        std::ifstream stat_file("/proc/self/stat");
        if (stat_file.is_open()) {
            std::getline(stat_file, stat_content);
            std::cout << "Read live /proc/self/stat from Linux host:\n" << stat_content << "\n\n";
        } else {
            std::cout << "Note: Could not open /proc/self/stat (" << std::strerror(errno) << ").\n";
            std::cout << "Host is likely not Linux (e.g., macOS Darwin).\n";
            std::cout << "Falling back to a realistic Linux 6.x sample string with a complex process name:\n";
            // Realistic Linux sample with spaces and parentheses in comm to test edge cases
            stat_content = "14285 (dbus-daemon (worker)) S 1 14285 14285 0 -1 4194304 420 0 0 0 15 25 0 0 20 0 1 0 351280 8192000 780 18446744073709551615 1 1 0 0 0 0 0 0 0 0 0 0 0 17 0 0 0 0 0 0";
            std::cout << stat_content << "\n\n";
        }
    }

    StatFields parsed;
    if (parse_stat_line(stat_content, parsed)) {
        std::cout << "Successfully parsed fields:\n";
        std::cout << "  Field  1 (pid):         " << parsed.pid << "\n";
        std::cout << "  Field  2 (comm):        [" << parsed.comm << "]\n";
        std::cout << "  Field  3 (state):       " << parsed.state << "\n";
        std::cout << "  Field  4 (ppid):        " << parsed.ppid << "\n";
        std::cout << "  Field 14 (utime):       " << parsed.utime << " ticks ("
                  << (clk_tck > 0 ? static_cast<double>(parsed.utime) / static_cast<double>(clk_tck) : 0.0)
                  << " s)\n";
        std::cout << "  Field 15 (stime):       " << parsed.stime << " ticks ("
                  << (clk_tck > 0 ? static_cast<double>(parsed.stime) / static_cast<double>(clk_tck) : 0.0)
                  << " s)\n";
        std::cout << "  Field 20 (num_threads): " << parsed.num_threads << "\n";
        std::cout << "  Field 22 (starttime):   " << parsed.starttime << " ticks after boot\n";
    } else {
        std::cerr << "Failed to parse stat line!\n";
        return 1;
    }

    return 0;
}
