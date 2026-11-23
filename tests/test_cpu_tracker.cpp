#include <gtest/gtest.h>
#include "cpu_tracker.hpp"
#include <thread>

TEST(CpuTrackerTest, StandardDeltaCalculation) {
    minimon::CpuTracker tracker(100); // 100 ticks per second

    // Case: Process used 50 ticks in 1.0 second = 0.50 CPU seconds / 1.0s = 50.0%
    double cpu = tracker.calculate_cpu_percent(100, 150, 1000, 1000, 1.0);
    EXPECT_DOUBLE_EQ(cpu, 50.0);

    // Case: Process used 100 ticks in 1.0 second = 1.00 CPU seconds / 1.0s = 100.0%
    cpu = tracker.calculate_cpu_percent(100, 200, 1000, 1000, 1.0);
    EXPECT_DOUBLE_EQ(cpu, 100.0);

    // Case: Multithreaded process used 200 ticks in 1.0 second = 200.0%
    cpu = tracker.calculate_cpu_percent(100, 300, 1000, 1000, 1.0);
    EXPECT_DOUBLE_EQ(cpu, 200.0);
}

TEST(CpuTrackerTest, CounterDidNotAdvance) {
    minimon::CpuTracker tracker(100);
    // Process was idle or sleeping: tick counter did not increment
    double cpu = tracker.calculate_cpu_percent(500, 500, 1000, 1000, 1.0);
    EXPECT_DOUBLE_EQ(cpu, 0.0);
}

TEST(CpuTrackerTest, CounterWentBackwards) {
    minimon::CpuTracker tracker(100);
    // Anomaly/wrap: counter goes backwards
    double cpu = tracker.calculate_cpu_percent(500, 480, 1000, 1000, 1.0);
    EXPECT_DOUBLE_EQ(cpu, 0.0);
}

TEST(CpuTrackerTest, ZeroOrNegativeElapsedTime) {
    minimon::CpuTracker tracker(100);
    // Timer anomaly: elapsed time is zero or negative
    EXPECT_DOUBLE_EQ(tracker.calculate_cpu_percent(100, 150, 1000, 1000, 0.0), 0.0);
    EXPECT_DOUBLE_EQ(tracker.calculate_cpu_percent(100, 150, 1000, 1000, -0.5), 0.0);
}

TEST(CpuTrackerTest, PidReuseDetection) {
    minimon::CpuTracker tracker(100);
    // Original process (starttime = 1000) exited, new process took over PID (starttime = 2500)
    double cpu = tracker.calculate_cpu_percent(100, 150, 1000, 2500, 1.0);
    EXPECT_DOUBLE_EQ(cpu, 0.0);
}

TEST(CpuTrackerTest, StatefulProcessLifecycle) {
    minimon::CpuTracker tracker(100);

    auto t0 = std::chrono::steady_clock::now();
    minimon::Process p;
    p.pid = 42;
    p.starttime = 5000;
    p.utime = 10;
    p.stime = 10;

    // Cycle 1: initial observation (no prior sample)
    tracker.update_process_cpu(p, t0);
    EXPECT_DOUBLE_EQ(p.cpu_percent, 0.0);

    // Cycle 2: 1.0s later, utime + stime advanced from 20 to 70 (+50 ticks = 0.5s CPU)
    auto t1 = t0 + std::chrono::seconds(1);
    p.utime = 35;
    p.stime = 35;
    tracker.update_process_cpu(p, t1);
    EXPECT_DOUBLE_EQ(p.cpu_percent, 50.0);

    // Cycle 3: 0.5s later, process used 25 ticks (= 0.25s CPU in 0.5s = 50.0%)
    auto t2 = t1 + std::chrono::milliseconds(500);
    p.utime = 45;
    p.stime = 50; // total 95 ticks (+25 ticks)
    tracker.update_process_cpu(p, t2);
    EXPECT_DOUBLE_EQ(p.cpu_percent, 50.0);

    // Cycle 4: PID reuse simulation on PID 42 (new starttime = 9000)
    auto t3 = t2 + std::chrono::seconds(1);
    p.starttime = 9000;
    p.utime = 5;
    p.stime = 5;
    tracker.update_process_cpu(p, t3);
    EXPECT_DOUBLE_EQ(p.cpu_percent, 0.0); // Reset to 0% due to PID reuse
}

TEST(CpuTrackerTest, PruneExitedProcesses) {
    minimon::CpuTracker tracker(100);
    auto now = std::chrono::steady_clock::now();

    minimon::Process p1; p1.pid = 1; tracker.update_process_cpu(p1, now);
    minimon::Process p2; p2.pid = 2; tracker.update_process_cpu(p2, now);
    minimon::Process p3; p3.pid = 3; tracker.update_process_cpu(p3, now);

    EXPECT_EQ(tracker.tracked_count(), 3u);

    // Simulate processes 1 and 3 still alive, process 2 exited
    tracker.prune_exited({1, 3});
    EXPECT_EQ(tracker.tracked_count(), 2u);

    // Ensure clear resets all
    tracker.clear();
    EXPECT_EQ(tracker.tracked_count(), 0u);
}
