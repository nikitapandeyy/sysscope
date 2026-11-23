#include "cli.hpp"
#include <iostream>
#include <sstream>

namespace minimon {

void print_help(std::ostream& os) {
    os << "Usage: minimon [OPTIONS]\n\n"
       << "Mini Process Monitor - Linux /proc Inspector in C++17\n\n"
       << "Options:\n"
       << "  -h, --help               Display this help text and exit\n"
       << "  --once                   Print process table once and exit (default: live view)\n"
       << "  --json                   Export process snapshot as JSON\n"
       << "  -s, --search <str>       Filter processes by name or command line substring\n"
       << "  -u, --user <username>    Filter processes by owning user name\n"
       << "  --state <R|S|D|Z|T>      Filter processes by execution state\n"
       << "  -p, --pid <pid>          Filter by specific Process ID\n"
       << "  --min-cpu <percent>      Filter processes consuming at least X% CPU\n"
       << "  --min-mem <MB>           Filter processes consuming at least Y MB RSS RAM\n"
       << "  --sort <field>           Sort by field: cpu, mem, pid, name (default: cpu)\n"
       << "  --asc                    Sort in ascending order\n"
       << "  --desc                   Sort in descending order (default)\n"
       << "  -n, --top <count>        Number of processes to display (default: 20, 0 for all)\n"
       << "  -i, --interval <sec>     Sampling refresh interval in seconds (default: 1.0)\n"
       << "  --proc-root <path>       Custom procfs mount directory (default: /proc)\n\n"
       << "Examples:\n"
       << "  minimon                  # Interactive live view of top 20 processes by CPU\n"
       << "  minimon --once           # One-shot report of top processes\n"
       << "  minimon --search chrome  # Filter processes containing 'chrome'\n"
       << "  minimon --sort mem --top 10  # Top 10 memory-consuming processes\n"
       << "  minimon --user root --state S # Sleeping root processes\n";
}

bool parse_cli_args(int argc, const char* const argv[], CliConfig& config) {
    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            config.show_help = true;
            return true;
        } else if (arg == "--once") {
            config.once = true;
        } else if (arg == "--json") {
            config.json = true;
        } else if (arg == "--asc") {
            config.sort_order = SortOrder::Ascending;
        } else if (arg == "--desc") {
            config.sort_order = SortOrder::Descending;
        } else if (arg == "-s" || arg == "--search") {
            if (i + 1 >= argc) {
                config.error_message = "Missing argument for " + std::string(arg);
                return false;
            }
            config.filter.name_search = argv[++i];
        } else if (arg == "-u" || arg == "--user") {
            if (i + 1 >= argc) {
                config.error_message = "Missing argument for " + std::string(arg);
                return false;
            }
            config.filter.user = argv[++i];
        } else if (arg == "--state") {
            if (i + 1 >= argc) {
                config.error_message = "Missing argument for --state";
                return false;
            }
            std::string st = argv[++i];
            if (st.empty()) {
                config.error_message = "Empty state argument";
                return false;
            }
            config.filter.state = st[0];
        } else if (arg == "-p" || arg == "--pid") {
            if (i + 1 >= argc) {
                config.error_message = "Missing argument for " + std::string(arg);
                return false;
            }
            try {
                config.filter.pid = static_cast<pid_t>(std::stoi(argv[++i]));
            } catch (...) {
                config.error_message = "Invalid integer for PID";
                return false;
            }
        } else if (arg == "--min-cpu") {
            if (i + 1 >= argc) {
                config.error_message = "Missing argument for --min-cpu";
                return false;
            }
            try {
                config.filter.min_cpu = std::stod(argv[++i]);
            } catch (...) {
                config.error_message = "Invalid number for --min-cpu";
                return false;
            }
        } else if (arg == "--min-mem") {
            if (i + 1 >= argc) {
                config.error_message = "Missing argument for --min-mem";
                return false;
            }
            try {
                config.filter.min_mem_mb = std::stod(argv[++i]);
            } catch (...) {
                config.error_message = "Invalid number for --min-mem";
                return false;
            }
        } else if (arg == "--sort") {
            if (i + 1 >= argc) {
                config.error_message = "Missing argument for --sort";
                return false;
            }
            std::string field_str = argv[++i];
            auto sf = parse_sort_field(field_str);
            if (!sf.has_value()) {
                config.error_message = "Invalid sort field: '" + field_str + "' (expected: cpu, mem, pid, name)";
                return false;
            }
            config.sort_field = *sf;
        } else if (arg == "-n" || arg == "--top") {
            if (i + 1 >= argc) {
                config.error_message = "Missing argument for " + std::string(arg);
                return false;
            }
            try {
                config.top_n = std::stoul(argv[++i]);
            } catch (...) {
                config.error_message = "Invalid number for --top";
                return false;
            }
        } else if (arg == "-i" || arg == "--interval") {
            if (i + 1 >= argc) {
                config.error_message = "Missing argument for " + std::string(arg);
                return false;
            }
            try {
                config.interval = std::stod(argv[++i]);
                if (config.interval <= 0.0) {
                    config.error_message = "Interval must be greater than 0";
                    return false;
                }
            } catch (...) {
                config.error_message = "Invalid number for --interval";
                return false;
            }
        } else if (arg == "--proc-root") {
            if (i + 1 >= argc) {
                config.error_message = "Missing argument for --proc-root";
                return false;
            }
            config.proc_root = argv[++i];
        } else {
            config.error_message = "Unknown option: " + std::string(arg);
            return false;
        }
    }
    return true;
}

} // namespace minimon
