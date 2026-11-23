#include "filter.hpp"
#include <algorithm>
#include <cctype>

namespace minimon {

bool case_insensitive_contains(std::string_view haystack, std::string_view needle) noexcept {
    if (needle.empty()) {
        return true;
    }
    if (haystack.size() < needle.size()) {
        return false;
    }

    auto it = std::search(
        haystack.begin(), haystack.end(),
        needle.begin(), needle.end(),
        [](char ch1, char ch2) {
            return std::tolower(static_cast<unsigned char>(ch1)) ==
                   std::tolower(static_cast<unsigned char>(ch2));
        }
    );
    return it != haystack.end();
}

bool case_insensitive_equals(std::string_view a, std::string_view b) noexcept {
    if (a.size() != b.size()) {
        return false;
    }
    return std::equal(
        a.begin(), a.end(),
        b.begin(),
        [](char ch1, char ch2) {
            return std::tolower(static_cast<unsigned char>(ch1)) ==
                   std::tolower(static_cast<unsigned char>(ch2));
        }
    );
}

bool matches_filter(const Process& proc, const FilterOptions& opts) noexcept {
    // 1. PID filter
    if (opts.pid.has_value() && proc.pid != *opts.pid) {
        return false;
    }

    // 2. Name / cmdline substring search (case-insensitive)
    if (!opts.name_search.empty()) {
        bool in_name = case_insensitive_contains(proc.name, opts.name_search);
        bool in_cmdline = case_insensitive_contains(proc.cmdline, opts.name_search);
        if (!in_name && !in_cmdline) {
            return false;
        }
    }

    // 3. User filter
    if (!opts.user.empty() && !case_insensitive_equals(proc.user, opts.user)) {
        return false;
    }

    // 4. State filter
    if (opts.state != '\0') {
        char proc_state_upper = static_cast<char>(std::toupper(static_cast<unsigned char>(proc.state)));
        char opt_state_upper = static_cast<char>(std::toupper(static_cast<unsigned char>(opts.state)));
        if (proc_state_upper != opt_state_upper) {
            return false;
        }
    }

    // 5. Min CPU% threshold
    if (opts.min_cpu > 0.0 && proc.cpu_percent < opts.min_cpu) {
        return false;
    }

    // 6. Min RSS memory threshold (in MB)
    if (opts.min_mem_mb > 0.0 && proc.rss_mb() < opts.min_mem_mb) {
        return false;
    }

    return true;
}

std::vector<Process> apply_filter(const std::vector<Process>& procs, const FilterOptions& opts) {
    std::vector<Process> filtered;
    filtered.reserve(procs.size());

    for (const auto& proc : procs) {
        if (matches_filter(proc, opts)) {
            filtered.push_back(proc);
        }
    }

    return filtered;
}

} // namespace minimon
