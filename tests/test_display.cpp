#include <gtest/gtest.h>
#include "display.hpp"
#include <sstream>

TEST(DisplayTest, TruncateString) {
    EXPECT_EQ(minimon::truncate_string("hello", 10), "hello");
    EXPECT_EQ(minimon::truncate_string("hello", 5), "hello");
    EXPECT_EQ(minimon::truncate_string("hello world", 8), "hello...");
    EXPECT_EQ(minimon::truncate_string("hi", 2), "hi");
    EXPECT_EQ(minimon::truncate_string("abc", 2), "ab");
}

TEST(DisplayTest, FormatMemory) {
    EXPECT_EQ(minimon::format_memory(512), "512K");
    EXPECT_EQ(minimon::format_memory(2048), "2.0M");
    EXPECT_EQ(minimon::format_memory(1048576), "1.0G");
    EXPECT_EQ(minimon::format_memory(1572864), "1.5G");
}

TEST(DisplayTest, MakeSummary) {
    std::vector<minimon::Process> procs;

    minimon::Process p1; p1.state = 'R'; procs.push_back(p1);
    minimon::Process p2; p2.state = 'S'; procs.push_back(p2);
    minimon::Process p3; p3.state = 'D'; procs.push_back(p3); // disk sleep counted with sleeping
    minimon::Process p4; p4.state = 'Z'; procs.push_back(p4);
    minimon::Process p5; p5.state = 'T'; procs.push_back(p5);

    minimon::SystemMemory mem;
    mem.total_kb = 16384000;
    mem.available_kb = 12288000;

    auto summary = minimon::make_summary(procs, mem);
    EXPECT_EQ(summary.total_tasks, 5u);
    EXPECT_EQ(summary.running_tasks, 1u);
    EXPECT_EQ(summary.sleeping_tasks, 2u); // S + D
    EXPECT_EQ(summary.zombie_tasks, 1u);
    EXPECT_EQ(summary.stopped_tasks, 1u);
}

TEST(DisplayTest, RenderTableOutput) {
    std::vector<minimon::Process> procs;

    minimon::Process p1;
    p1.pid = 1;
    p1.ppid = 0;
    p1.name = "systemd";
    p1.user = "root";
    p1.state = 'S';
    p1.cpu_percent = 0.5;
    p1.rss_kb = 8192;
    p1.mem_percent = 0.1;
    p1.threads = 1;
    p1.cmdline = "/sbin/init";
    procs.push_back(p1);

    minimon::Process p2;
    p2.pid = 100;
    p2.ppid = 1;
    p2.name = "bash";
    p2.user = "nikita";
    p2.state = 'R';
    p2.cpu_percent = 25.0;
    p2.rss_kb = 16384;
    p2.mem_percent = 0.2;
    p2.threads = 2;
    p2.cmdline = "/bin/bash --login -i";
    procs.push_back(p2);

    minimon::SystemMemory mem;
    mem.total_kb = 16384000;
    mem.available_kb = 12288000;
    auto summary = minimon::make_summary(procs, mem);

    std::ostringstream oss;
    minimon::render_table(oss, procs, summary, 0, 80);
    std::string output = oss.str();

    EXPECT_NE(output.find("Tasks: 2 total"), std::string::npos);
    EXPECT_NE(output.find("PID"), std::string::npos);
    EXPECT_NE(output.find("COMMAND"), std::string::npos);
    EXPECT_NE(output.find("/sbin/init"), std::string::npos);
    EXPECT_NE(output.find("/bin/bash"), std::string::npos);
}

TEST(DisplayTest, MaxRowsLimiting) {
    std::vector<minimon::Process> procs;
    for (int i = 1; i <= 10; ++i) {
        minimon::Process p;
        p.pid = i;
        p.name = "proc" + std::to_string(i);
        p.cmdline = p.name;
        procs.push_back(p);
    }

    minimon::SystemMemory mem;
    auto summary = minimon::make_summary(procs, mem);

    std::ostringstream oss;
    minimon::render_table(oss, procs, summary, 3, 80); // max 3 rows
    std::string output = oss.str();

    EXPECT_NE(output.find("proc1"), std::string::npos);
    EXPECT_NE(output.find("proc2"), std::string::npos);
    EXPECT_NE(output.find("proc3"), std::string::npos);
    EXPECT_EQ(output.find("proc4"), std::string::npos); // Should not appear
}

#include "cli.hpp"

TEST(DisplayLiveTest, SignalHandlingAndStopRequest) {
    minimon::setup_signal_handlers();
    EXPECT_FALSE(minimon::is_stop_requested());

    minimon::request_stop();
    EXPECT_TRUE(minimon::is_stop_requested());
}

TEST(DisplayLiveTest, RunSingleShotOnFixture) {
    minimon::CliConfig config;
    config.once = true;
    config.proc_root = std::string(TEST_FIXTURES_DIR) + "/proc_full";

    std::ostringstream oss;
    minimon::run_single_shot(config, oss);
    std::string out = oss.str();

    EXPECT_NE(out.find("/sbin/init"), std::string::npos);
    EXPECT_NE(out.find("/bin/bash"), std::string::npos);
    EXPECT_NE(out.find("[kthreadd]"), std::string::npos);
}

TEST(DisplayLiveTest, RunLiveLoopExitsOnStop) {
    minimon::CliConfig config;
    config.proc_root = std::string(TEST_FIXTURES_DIR) + "/proc_full";
    config.interval = 0.05;

    // Set stop requested before loop runs so it iterates once and exits
    minimon::request_stop();

    std::ostringstream oss;
    minimon::run_live_loop(config, oss);
    std::string out = oss.str();

    EXPECT_NE(out.find("\033[2J\033[H"), std::string::npos);
}

TEST(DisplayTest, JsonEscape) {
    EXPECT_EQ(minimon::json_escape("hello"), "hello");
    EXPECT_EQ(minimon::json_escape("hello \"world\""), "hello \\\"world\\\"");
    EXPECT_EQ(minimon::json_escape("line1\nline2\ttab"), "line1\\nline2\\ttab");
    EXPECT_EQ(minimon::json_escape("path\\to\\bin"), "path\\\\to\\\\bin");
}

TEST(DisplayTest, RenderJsonOutput) {
    std::vector<minimon::Process> procs;

    minimon::Process p1;
    p1.pid = 1;
    p1.ppid = 0;
    p1.name = "systemd";
    p1.user = "root";
    p1.uid = 0;
    p1.state = 'S';
    p1.cpu_percent = 0.5;
    p1.rss_kb = 8192;
    p1.vm_size_kb = 16384;
    p1.mem_percent = 0.05;
    p1.threads = 1;
    p1.cmdline = "/sbin/init splash";
    procs.push_back(p1);

    minimon::SystemMemory mem;
    mem.total_kb = 16384000;
    mem.free_kb = 8000000;
    mem.available_kb = 12000000;
    auto summary = minimon::make_summary(procs, mem);

    std::string json = minimon::render_json(procs, summary, 0);

    EXPECT_NE(json.find("\"schema_version\": \"1.0\""), std::string::npos);
    EXPECT_NE(json.find("\"system\": {"), std::string::npos);
    EXPECT_NE(json.find("\"total\": 1"), std::string::npos);
    EXPECT_NE(json.find("\"processes\": ["), std::string::npos);
    EXPECT_NE(json.find("\"pid\": 1"), std::string::npos);
    EXPECT_NE(json.find("\"name\": \"systemd\""), std::string::npos);
    EXPECT_NE(json.find("\"cmdline\": \"/sbin/init splash\""), std::string::npos);
}

TEST(DisplayLiveTest, RunSingleShotJsonOnFixture) {
    minimon::CliConfig config;
    config.once = true;
    config.json = true;
    config.proc_root = std::string(TEST_FIXTURES_DIR) + "/proc_full";

    std::ostringstream oss;
    minimon::run_single_shot(config, oss);
    std::string out = oss.str();

    EXPECT_NE(out.find("\"schema_version\": \"1.0\""), std::string::npos);
    EXPECT_NE(out.find("\"pid\": 1"), std::string::npos);
    EXPECT_NE(out.find("\"cmdline\": \"/sbin/init\""), std::string::npos);
    EXPECT_NE(out.find("\"name\": \"kthreadd\""), std::string::npos);
}
