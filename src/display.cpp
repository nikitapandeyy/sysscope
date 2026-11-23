#include "display.hpp"
#include "cli.hpp"
#include "cpu_tracker.hpp"
#include "filter.hpp"
#include <iomanip>
#include <sstream>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstdlib>
#include <cctype>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

namespace minimon {

namespace {

std::atomic<bool> g_stop_requested{false};

void handle_signal(int /*sig*/) noexcept {
    g_stop_requested.store(true, std::memory_order_relaxed);
}

// RAII Terminal guard to hide cursor on start and restore cursor on exit
struct TerminalGuard {
    std::ostream& os;
    bool active{false};

    explicit TerminalGuard(std::ostream& out, bool is_interactive)
        : os(out), active(is_interactive) {
        if (active) {
            os << "\033[?25l"; // Hide cursor
            os.flush();
        }
    }

    ~TerminalGuard() {
        if (active) {
            os << "\033[?25h\033[0m\n"; // Show cursor and reset attributes
            os.flush();
        }
    }
};

} // anonymous namespace

void setup_signal_handlers() noexcept {
    struct sigaction sa{};
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
}

bool is_stop_requested() noexcept {
    return g_stop_requested.load(std::memory_order_relaxed);
}

void request_stop() noexcept {
    g_stop_requested.store(true, std::memory_order_relaxed);
}

size_t get_terminal_width() noexcept {
    struct winsize ws{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) {
        return ws.ws_col;
    }
    const char* env_cols = std::getenv("COLUMNS");
    if (env_cols) {
        int c = std::atoi(env_cols);
        if (c > 20) {
            return static_cast<size_t>(c);
        }
    }
    return 100; // Default standard terminal width
}

void get_system_loadavg(double& l1, double& l5, double& l15) noexcept {
    double loads[3] = {0.0, 0.0, 0.0};
    if (getloadavg(loads, 3) >= 1) {
        l1 = loads[0];
        l5 = loads[1];
        l15 = loads[2];
    } else {
        l1 = l5 = l15 = 0.0;
    }
}

SystemSummary make_summary(const std::vector<Process>& procs, const SystemMemory& mem) {
    SystemSummary s;
    s.total_tasks = procs.size();
    s.memory = mem;

    for (const auto& p : procs) {
        char st = static_cast<char>(std::toupper(static_cast<unsigned char>(p.state)));
        if (st == 'R') {
            s.running_tasks++;
        } else if (st == 'S' || st == 'D' || st == 'I') {
            s.sleeping_tasks++;
        } else if (st == 'Z') {
            s.zombie_tasks++;
        } else if (st == 'T' || st == 't') {
            s.stopped_tasks++;
        }
    }

    get_system_loadavg(s.load_1m, s.load_5m, s.load_15m);
    return s;
}

std::string truncate_string(std::string_view str, size_t max_len) {
    if (str.size() <= max_len) {
        return std::string(str);
    }
    if (max_len <= 3) {
        return std::string(str.substr(0, max_len));
    }
    return std::string(str.substr(0, max_len - 3)) + "...";
}

std::string format_memory(uint64_t kb) {
    std::ostringstream oss;
    if (kb >= 1024 * 1024) {
        double gb = static_cast<double>(kb) / (1024.0 * 1024.0);
        oss << std::fixed << std::setprecision(1) << gb << "G";
    } else if (kb >= 1024) {
        double mb = static_cast<double>(kb) / 1024.0;
        oss << std::fixed << std::setprecision(1) << mb << "M";
    } else {
        oss << kb << "K";
    }
    return oss.str();
}

void render_table(
    std::ostream& os,
    const std::vector<Process>& procs,
    const SystemSummary& summary,
    size_t max_rows,
    size_t term_width
) {
    if (term_width == 0) {
        term_width = get_terminal_width();
    }

    // 1. Summary Header
    os << "Tasks: " << summary.total_tasks << " total, "
       << summary.running_tasks << " running, "
       << summary.sleeping_tasks << " sleeping, "
       << summary.zombie_tasks << " zombie, "
       << summary.stopped_tasks << " stopped\n";

    if (summary.memory.total_kb > 0) {
        os << "Mem:   " << std::fixed << std::setprecision(1)
           << summary.memory.total_mb() << "M total, "
           << summary.memory.used_mb() << "M used ("
           << summary.memory.used_percent() << "%), "
           << (static_cast<double>(summary.memory.available_kb) / 1024.0) << "M avail, "
           << (static_cast<double>(summary.memory.swap_total_kb - summary.memory.swap_free_kb) / 1024.0) << "M/"
           << (static_cast<double>(summary.memory.swap_total_kb) / 1024.0) << "M swap\n";
    }

    os << "Load:  " << std::fixed << std::setprecision(2)
       << summary.load_1m << ", " << summary.load_5m << ", " << summary.load_15m << "\n\n";

    // 2. Table Column Headers
    // Fixed width prefix:
    // PID(7) + PPID(7) + USER(10) + STAT(4) + %CPU(6) + %MEM(6) + RSS(9) + THR(4) + 8 spaces = 61 chars
    os << std::right << std::setw(7) << "PID" << " "
       << std::right << std::setw(7) << "PPID" << " "
       << std::left  << std::setw(10) << "USER" << " "
       << std::left  << std::setw(4) << "STAT" << " "
       << std::right << std::setw(6) << "%CPU" << " "
       << std::right << std::setw(6) << "%MEM" << " "
       << std::right << std::setw(9) << "RSS" << " "
       << std::right << std::setw(4) << "THR" << " "
       << std::left  << "COMMAND\n";

    size_t fixed_width = 7 + 1 + 7 + 1 + 10 + 1 + 4 + 1 + 6 + 1 + 6 + 1 + 9 + 1 + 4 + 1; // 61
    size_t cmd_width = (term_width > fixed_width) ? (term_width - fixed_width) : 20;

    // Divider line
    os << std::string(term_width, '-') << "\n";

    // 3. Process Rows
    size_t count = 0;
    for (const auto& p : procs) {
        if (max_rows > 0 && count >= max_rows) {
            break;
        }

        std::string user_str = truncate_string(p.user, 10);
        std::string rss_str = format_memory(p.rss_kb);
        std::string cmd_str = truncate_string(p.cmdline.empty() ? ("[" + p.name + "]") : p.cmdline, cmd_width);

        os << std::right << std::setw(7) << p.pid << " "
           << std::right << std::setw(7) << p.ppid << " "
           << std::left  << std::setw(10) << user_str << " "
           << std::left  << std::setw(4) << p.state << " "
           << std::right << std::setw(6) << std::fixed << std::setprecision(1) << p.cpu_percent << " "
           << std::right << std::setw(6) << std::fixed << std::setprecision(1) << p.mem_percent << " "
           << std::right << std::setw(9) << rss_str << " "
           << std::right << std::setw(4) << p.threads << " "
           << std::left  << cmd_str << "\n";

        count++;
    }
}

void run_single_shot(const CliConfig& config, std::ostream& os) {
    ProcReader reader(config.proc_root);
    CpuTracker tracker;

    // Take initial sample
    static_cast<void>(reader.read_all_processes(tracker));

    // If running with mock fixtures with 0 interval or non-standard root, skip sleep
    if (config.proc_root == "/proc") {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    // Take measurement sample
    auto procs = reader.read_all_processes(tracker);

    // Apply filtering
    auto filtered = apply_filter(procs, config.filter);

    // Apply sorting
    sort_processes(filtered, config.sort_field, config.sort_order);

    // Read system memory
    SystemMemory mem;
    static_cast<void>(reader.read_meminfo(mem));

    // Render table
    auto summary = make_summary(procs, mem);
    render_table(os, filtered, summary, config.top_n);
}

void run_live_loop(const CliConfig& config, std::ostream& os) {
    setup_signal_handlers();
    TerminalGuard guard(os, isatty(STDOUT_FILENO) != 0);

    ProcReader reader(config.proc_root);
    CpuTracker tracker;

    // Clear screen once at startup
    os << "\033[2J\033[H";
    os.flush();

    while (!is_stop_requested()) {
        auto procs = reader.read_all_processes(tracker);
        auto filtered = apply_filter(procs, config.filter);
        sort_processes(filtered, config.sort_field, config.sort_order);

        SystemMemory mem;
        static_cast<void>(reader.read_meminfo(mem));
        auto summary = make_summary(procs, mem);

        // Move cursor to home position without clearing screen to avoid flicker
        os << "\033[H";
        render_table(os, filtered, summary, config.top_n);
        os.flush();

        // Responsive sleep in 50ms increments to exit immediately on SIGINT
        auto sleep_total_ms = static_cast<int>(config.interval * 1000.0);
        int elapsed_ms = 0;
        while (elapsed_ms < sleep_total_ms && !is_stop_requested()) {
            int step = std::min(50, sleep_total_ms - elapsed_ms);
            std::this_thread::sleep_for(std::chrono::milliseconds(step));
            elapsed_ms += step;
        }
    }
}

} // namespace minimon
