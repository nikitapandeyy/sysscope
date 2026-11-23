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

std::optional<SortField> parse_sort_field(std::string_view str) noexcept {
    if (case_insensitive_equals(str, "cpu")) {
        return SortField::Cpu;
    }
    if (case_insensitive_equals(str, "mem") || case_insensitive_equals(str, "memory") || case_insensitive_equals(str, "rss")) {
        return SortField::Memory;
    }
    if (case_insensitive_equals(str, "pid")) {
        return SortField::Pid;
    }
    if (case_insensitive_equals(str, "name") || case_insensitive_equals(str, "comm")) {
        return SortField::Name;
    }
    return std::nullopt;
}

void sort_processes(std::vector<Process>& procs, SortField field, SortOrder order) {
    std::stable_sort(procs.begin(), procs.end(), [field, order](const Process& a, const Process& b) {
        if (order == SortOrder::Ascending) {
            switch (field) {
                case SortField::Cpu:
                    if (a.cpu_percent != b.cpu_percent) return a.cpu_percent < b.cpu_percent;
                    return a.pid < b.pid;
                case SortField::Memory:
                    if (a.rss_kb != b.rss_kb) return a.rss_kb < b.rss_kb;
                    return a.pid < b.pid;
                case SortField::Pid:
                    return a.pid < b.pid;
                case SortField::Name:
                    if (a.name != b.name) return a.name < b.name;
                    return a.pid < b.pid;
            }
        } else {
            // Descending order (standard top behavior)
            switch (field) {
                case SortField::Cpu:
                    if (a.cpu_percent != b.cpu_percent) return a.cpu_percent > b.cpu_percent;
                    return a.pid < b.pid;
                case SortField::Memory:
                    if (a.rss_kb != b.rss_kb) return a.rss_kb > b.rss_kb;
                    return a.pid < b.pid;
                case SortField::Pid:
                    return a.pid > b.pid;
                case SortField::Name:
                    if (a.name != b.name) return a.name > b.name;
                    return a.pid < b.pid;
            }
        }
        return false;
    });
}

} // namespace minimon
