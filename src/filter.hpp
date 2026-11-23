#pragma once

#include <string>
#include <vector>
#include <optional>
#include <string_view>
#include "process.hpp"

namespace minimon {

// Criteria used to filter process lists.
struct FilterOptions {
    std::string name_search;          // Case-insensitive substring match on process name or cmdline
    std::optional<pid_t> pid;         // Exact PID filter
    std::string user;                 // Case-insensitive user name match
    char state{'\0'};                 // Single character state filter ('R', 'S', 'Z', etc., or '\0' for all)
    double min_cpu{0.0};              // Minimum CPU% threshold
    double min_mem_mb{0.0};           // Minimum RSS memory threshold in MB
};

// Case-insensitive substring search helper.
[[nodiscard]] bool case_insensitive_contains(std::string_view haystack, std::string_view needle) noexcept;

// Case-insensitive string equality helper.
[[nodiscard]] bool case_insensitive_equals(std::string_view a, std::string_view b) noexcept;

// Check whether a single Process matches the given FilterOptions.
[[nodiscard]] bool matches_filter(const Process& proc, const FilterOptions& opts) noexcept;

// Filter a collection of processes, returning only those matching all specified criteria.
[[nodiscard]] std::vector<Process> apply_filter(const std::vector<Process>& procs, const FilterOptions& opts);

// Fields by which process lists can be sorted.
enum class SortField {
    Cpu,
    Memory,
    Pid,
    Name
};

// Sort ordering direction.
enum class SortOrder {
    Descending,
    Ascending
};

// Parse a string (e.g. "cpu", "mem", "pid", "name") into a SortField.
[[nodiscard]] std::optional<SortField> parse_sort_field(std::string_view str) noexcept;

// Sort processes in-place by the specified field and order.
void sort_processes(
    std::vector<Process>& procs,
    SortField field = SortField::Cpu,
    SortOrder order = SortOrder::Descending
);

} // namespace minimon
