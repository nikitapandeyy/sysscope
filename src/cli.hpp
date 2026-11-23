#pragma once

#include <string>
#include <vector>
#include <optional>
#include "filter.hpp"

namespace minimon {

// Structured command-line configuration for minimon.
struct CliConfig {
    bool once{false};                            // One-shot snapshot output instead of live refresh
    bool json{false};                            // Output in structured JSON format
    bool show_help{false};                       // Help flag was requested
    double interval{1.0};                        // Refresh interval in seconds (default 1.0)
    size_t top_n{20};                            // Display top N processes (default 20, 0 for all)
    SortField sort_field{SortField::Cpu};        // Sort column (default: CPU%)
    SortOrder sort_order{SortOrder::Descending}; // Sort order (default: Descending)
    FilterOptions filter;                        // Search and filter criteria
    std::string proc_root{"/proc"};              // Procfs root directory
    std::string error_message;                   // Error message if parsing failed
};

// Parse command line arguments into CliConfig.
// Returns true on successful parsing or --help; returns false on invalid arguments.
[[nodiscard]] bool parse_cli_args(int argc, const char* const argv[], CliConfig& config);

// Print the formatted help documentation to the specified output stream.
void print_help(std::ostream& os);

} // namespace minimon
