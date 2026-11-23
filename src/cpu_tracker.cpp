#include "cpu_tracker.hpp"
#include <unistd.h>
#include <algorithm>
#include <unordered_set>

namespace minimon {

CpuTracker::CpuTracker(long clk_tck) {
    if (clk_tck > 0) {
        clk_tck_ = clk_tck;
    } else {
        long sys_tck = sysconf(_SC_CLK_TCK);
        clk_tck_ = (sys_tck > 0) ? sys_tck : 100;
    }
}

double CpuTracker::calculate_cpu_percent(
    unsigned long prev_ticks,
    unsigned long curr_ticks,
    unsigned long long prev_starttime,
    unsigned long long curr_starttime,
    double elapsed_seconds
) const noexcept {
    // 1. Guard against non-positive elapsed time
    if (elapsed_seconds <= 0.0) {
        return 0.0;
    }

    // 2. Guard against PID reuse: if starttime changed, this is a completely different process
    if (prev_starttime != curr_starttime) {
        return 0.0;
    }

    // 3. Guard against counter not advancing or counter going backwards (clock jitter / wrap)
    if (curr_ticks <= prev_ticks) {
        return 0.0;
    }

    unsigned long delta_ticks = curr_ticks - prev_ticks;
    double delta_cpu_seconds = static_cast<double>(delta_ticks) / static_cast<double>(clk_tck_);

    // CPU% = (CPU seconds used / wall-clock seconds elapsed) * 100.0
    double percent = (delta_cpu_seconds / elapsed_seconds) * 100.0;
    return percent;
}

void CpuTracker::update_process_cpu(Process& proc, std::chrono::steady_clock::time_point now) {
    unsigned long curr_ticks = proc.utime + proc.stime;
    auto it = history_.find(proc.pid);

    if (it == history_.end()) {
        // First time seeing this process: no previous sample exists
        proc.cpu_percent = 0.0;
        history_[proc.pid] = ProcessCpuSample{curr_ticks, proc.starttime, now};
        return;
    }

    const ProcessCpuSample& prev = it->second;
    std::chrono::duration<double> elapsed = now - prev.timestamp;

    proc.cpu_percent = calculate_cpu_percent(
        prev.total_ticks,
        curr_ticks,
        prev.starttime,
        proc.starttime,
        elapsed.count()
    );

    // Update historical sample for next iteration
    it->second = ProcessCpuSample{curr_ticks, proc.starttime, now};
}

void CpuTracker::prune_exited(const std::vector<pid_t>& active_pids) {
    std::unordered_set<pid_t> active_set(active_pids.begin(), active_pids.end());
    for (auto it = history_.begin(); it != history_.end();) {
        if (active_set.find(it->first) == active_set.end()) {
            it = history_.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace minimon
