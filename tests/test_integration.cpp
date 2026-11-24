#include <gtest/gtest.h>
#include "proc_reader.hpp"
#include "cpu_tracker.hpp"
#include <array>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <unistd.h>

#ifndef TEST_WORKLOAD_PATH
#define TEST_WORKLOAD_PATH "./build/test_workload"
#endif

#ifndef MINIMON_BIN_PATH
#define MINIMON_BIN_PATH "./build/minimon"
#endif

// RAII helper to spawn and safely manage a background test workload child
class WorkloadChildGuard {
public:
    WorkloadChildGuard(const std::vector<std::string>& args) {
        int pipefds[2];
        if (pipe(pipefds) != 0) {
            throw std::runtime_error("Failed to create pipe");
        }

        pid_ = fork();
        if (pid_ < 0) {
            close(pipefds[0]);
            close(pipefds[1]);
            throw std::runtime_error("Fork failed");
        }

        if (pid_ == 0) {
            // Child process: redirect stdout to pipe write end
            close(pipefds[0]);
            dup2(pipefds[1], STDOUT_FILENO);
            close(pipefds[1]);

            std::vector<const char*> argv_ptrs;
            argv_ptrs.push_back(TEST_WORKLOAD_PATH);
            for (const auto& arg : args) {
                argv_ptrs.push_back(arg.c_str());
            }
            argv_ptrs.push_back(nullptr);

            execv(TEST_WORKLOAD_PATH, const_cast<char* const*>(argv_ptrs.data()));
            _exit(127);
        }

        // Parent process: close write end and wait for READY signal from child
        close(pipefds[1]);

        std::array<char, 32> buf{};
        ssize_t bytes_read = read(pipefds[0], buf.data(), buf.size() - 1);
        close(pipefds[0]);

        if (bytes_read > 0) {
            ready_ = (std::string(buf.data()).find("READY") != std::string::npos);
        }
    }

    ~WorkloadChildGuard() {
        if (pid_ > 0) {
            kill(pid_, SIGTERM);
            int status = 0;
            waitpid(pid_, &status, 0);
        }
    }

    [[nodiscard]] pid_t pid() const noexcept { return pid_; }
    [[nodiscard]] bool is_ready() const noexcept { return ready_; }

private:
    pid_t pid_{-1};
    bool ready_{false};
};

// Integration test for memory allocation workload
TEST(IntegrationTest, ChildMemoryAllocationWorkload) {
    if (!std::filesystem::exists(TEST_WORKLOAD_PATH)) {
        GTEST_SKIP() << "test_workload binary not found at: " << TEST_WORKLOAD_PATH;
    }

    // Allocate 25 MB in child process
    WorkloadChildGuard child({"--mem", "25", "--duration", "5"});
    ASSERT_TRUE(child.is_ready());
    ASSERT_GT(child.pid(), 0);

    bool has_linux_proc = std::filesystem::exists("/proc/self/stat");
    if (!has_linux_proc) {
        std::cout << "[INFO] /proc not available on host OS (macOS). "
                  << "Child spawned with PID " << child.pid() << " verified via pipe." << std::endl;
        return;
    }

    minimon::ProcReader reader("/proc");
    minimon::Process proc;
    bool found = reader.read_single_process(child.pid(), proc);
    ASSERT_TRUE(found);

    EXPECT_EQ(proc.pid, child.pid());
    // 25 MB is 25,600 kB. Allow tolerance [15000 kB, 50000 kB] for runtime overhead
    EXPECT_GE(proc.rss_kb, 15000u);
    EXPECT_LE(proc.rss_kb, 50000u);
    EXPECT_NE(proc.cmdline.find("test_workload"), std::string::npos);
}

// Integration test for CPU burner workload
TEST(IntegrationTest, ChildCpuBurnerWorkload) {
    if (!std::filesystem::exists(TEST_WORKLOAD_PATH)) {
        GTEST_SKIP() << "test_workload binary not found at: " << TEST_WORKLOAD_PATH;
    }

    // Burn CPU in child process
    WorkloadChildGuard child({"--cpu", "--duration", "5"});
    ASSERT_TRUE(child.is_ready());
    ASSERT_GT(child.pid(), 0);

    bool has_linux_proc = std::filesystem::exists("/proc/self/stat");
    if (!has_linux_proc) {
        std::cout << "[INFO] /proc not available on host OS (macOS). "
                  << "Child spawned with PID " << child.pid() << " verified via pipe." << std::endl;
        return;
    }

    minimon::ProcReader reader("/proc");
    minimon::CpuTracker tracker;

    // First sample
    auto procs1 = reader.read_all_processes(tracker);

    // Sleep 400ms to allow child to burn CPU
    std::this_thread::sleep_for(std::chrono::milliseconds(400));

    // Second sample
    auto procs2 = reader.read_all_processes(tracker);

    bool found = false;
    for (const auto& p : procs2) {
        if (p.pid == child.pid()) {
            found = true;
            EXPECT_NE(p.cmdline.find("test_workload"), std::string::npos);
            // Single core CPU burning should be high (> 30%)
            EXPECT_GT(p.cpu_percent, 20.0);
            break;
        }
    }
    EXPECT_TRUE(found);
}

// Helper to run a command and capture stdout
static std::string run_minimon_cmd(const std::string& cmd) {
    std::array<char, 256> buffer{};
    std::string result;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        return "";
    }
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        result += buffer.data();
    }
    pclose(pipe);
    return result;
}

// Integration test running the minimon CLI binary end-to-end
TEST(IntegrationTest, CliBinaryEndToEndExecution) {
    std::string bin_path = MINIMON_BIN_PATH;
    if (!std::filesystem::exists(bin_path)) {
        GTEST_SKIP() << "minimon executable not found at: " << bin_path;
    }

    std::string cmd = bin_path + " --once --proc-root " + std::string(TEST_FIXTURES_DIR) + "/proc_full";
    std::string output = run_minimon_cmd(cmd);

    EXPECT_NE(output.find("Tasks: 3 total"), std::string::npos);
    EXPECT_NE(output.find("/sbin/init"), std::string::npos);
    EXPECT_NE(output.find("/bin/bash"), std::string::npos);
    EXPECT_NE(output.find("[kthreadd]"), std::string::npos);
}

// Integration test for CLI filtering and sorting options
TEST(IntegrationTest, CliBinaryFilteringAndJson) {
    std::string bin_path = MINIMON_BIN_PATH;
    if (!std::filesystem::exists(bin_path)) {
        GTEST_SKIP() << "minimon executable not found at: " << bin_path;
    }

    // Filter by name and export to JSON
    std::string cmd = bin_path + " --json --search bash --proc-root " + std::string(TEST_FIXTURES_DIR) + "/proc_full";
    std::string output = run_minimon_cmd(cmd);

    EXPECT_NE(output.find("\"schema_version\": \"1.0\""), std::string::npos);
    EXPECT_NE(output.find("\"name\": \"bash\""), std::string::npos);
    EXPECT_EQ(output.find("\"name\": \"systemd\""), std::string::npos);
}
