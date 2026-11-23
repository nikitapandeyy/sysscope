#include <gtest/gtest.h>
#include "proc_reader.hpp"
#include "cpu_tracker.hpp"

TEST(ProcReaderTest, ConfigurableRoot) {
    minimon::ProcReader reader("/custom/proc/path");
    EXPECT_EQ(reader.root(), "/custom/proc/path");
}

TEST(ProcReaderTest, EnumeratePidsFromFixture) {
    std::string fixture_dir = std::string(TEST_FIXTURES_DIR) + "/proc_enumeration";
    minimon::ProcReader reader(fixture_dir);
    std::vector<pid_t> pids = reader.get_pids();

    ASSERT_EQ(pids.size(), 3u);
    EXPECT_EQ(pids[0], 1);
    EXPECT_EQ(pids[1], 42);
    EXPECT_EQ(pids[2], 999);
}

TEST(ProcReaderTest, EnumeratePidsNonExistentDirectory) {
    minimon::ProcReader reader("tests/fixtures/does_not_exist");
    std::vector<pid_t> pids = reader.get_pids();
    EXPECT_TRUE(pids.empty());
}

TEST(ProcStatTest, StandardProcess) {
    std::string line = "1234 (bash) S 1000 1234 1234 0 -1 4194304 100 0 0 0 55 35 0 0 20 0 1 0 10000 16000 1200 18446744073709551615 1 1 0 0 0 0 0 0 0 0 0 0 0 17 0 0 0 0 0 0";
    minimon::Process p;
    ASSERT_TRUE(minimon::ProcReader::parse_stat_string(line, p));
    EXPECT_EQ(p.pid, 1234);
    EXPECT_EQ(p.name, "bash");
    EXPECT_EQ(p.state, 'S');
    EXPECT_EQ(p.ppid, 1000);
    EXPECT_EQ(p.utime, 55u);
    EXPECT_EQ(p.stime, 35u);
    EXPECT_EQ(p.threads, 1);
    EXPECT_EQ(p.starttime, 10000u);
}

TEST(ProcStatTest, ProcessNameWithSpaces) {
    // Process name "(a b)" contains spaces
    std::string line = "42 (a b) R 1 42 42 0 -1 4194304 10 0 0 0 12 18 0 0 20 0 2 0 20000 24000 2000 18446744073709551615 1 1 0 0 0 0 0 0 0 0 0 0 0 17 0 0 0 0 0 0";
    minimon::Process p;
    ASSERT_TRUE(minimon::ProcReader::parse_stat_string(line, p));
    EXPECT_EQ(p.pid, 42);
    EXPECT_EQ(p.name, "a b");
    EXPECT_EQ(p.state, 'R');
    EXPECT_EQ(p.ppid, 1);
    EXPECT_EQ(p.utime, 12u);
    EXPECT_EQ(p.stime, 18u);
    EXPECT_EQ(p.threads, 2);
    EXPECT_EQ(p.starttime, 20000u);
}

TEST(ProcStatTest, ProcessNameWithNestedParentheses) {
    // Complex process name "((a) b (c))" with multiple opening and closing parens
    std::string line = "99 ((a) b (c)) D 42 99 99 0 -1 4194304 20 0 0 0 45 65 0 0 20 0 8 0 30000 32000 3000 18446744073709551615 1 1 0 0 0 0 0 0 0 0 0 0 0 17 0 0 0 0 0 0";
    minimon::Process p;
    ASSERT_TRUE(minimon::ProcReader::parse_stat_string(line, p));
    EXPECT_EQ(p.pid, 99);
    EXPECT_EQ(p.name, "(a) b (c)");
    EXPECT_EQ(p.state, 'D');
    EXPECT_EQ(p.ppid, 42);
    EXPECT_EQ(p.utime, 45u);
    EXPECT_EQ(p.stime, 65u);
    EXPECT_EQ(p.threads, 8);
    EXPECT_EQ(p.starttime, 30000u);
}

TEST(ProcStatTest, ReadFromFixtureFiles) {
    std::string fixture_dir = std::string(TEST_FIXTURES_DIR) + "/proc_stat";
    minimon::ProcReader reader(fixture_dir);

    minimon::Process p100;
    ASSERT_TRUE(reader.read_stat(100, p100));
    EXPECT_EQ(p100.pid, 100);
    EXPECT_EQ(p100.name, "systemd");
    EXPECT_EQ(p100.state, 'S');
    EXPECT_EQ(p100.ppid, 0);
    EXPECT_EQ(p100.utime, 100u);
    EXPECT_EQ(p100.stime, 200u);

    minimon::Process p200;
    ASSERT_TRUE(reader.read_stat(200, p200));
    EXPECT_EQ(p200.pid, 200);
    EXPECT_EQ(p200.name, "a b");
    EXPECT_EQ(p200.state, 'R');
    EXPECT_EQ(p200.threads, 2);

    minimon::Process p300;
    ASSERT_TRUE(reader.read_stat(300, p300));
    EXPECT_EQ(p300.pid, 300);
    EXPECT_EQ(p300.name, "(a) b (c)");
    EXPECT_EQ(p300.state, 'S');
    EXPECT_EQ(p300.threads, 4);

    minimon::Process non_existent;
    EXPECT_FALSE(reader.read_stat(99999, non_existent));
}

TEST(ProcStatTest, MalformedLines) {
    minimon::Process p;
    // Missing parens entirely
    EXPECT_FALSE(minimon::ProcReader::parse_stat_string("1234 bash S 1 2 3", p));
    // Inverted parens
    EXPECT_FALSE(minimon::ProcReader::parse_stat_string("1234 )bash( S 1 2 3", p));
    // Missing fields
    EXPECT_FALSE(minimon::ProcReader::parse_stat_string("1234 (bash) S", p));
    // Non-numeric PID
    EXPECT_FALSE(minimon::ProcReader::parse_stat_string("abcd (bash) S 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20", p));
}

TEST(ProcStatusTest, ParseStandardStatusStream) {
    std::string content =
        "Name:\tpython3\n"
        "State:\tR (running)\n"
        "Tgid:\t4321\n"
        "Pid:\t4321\n"
        "PPid:\t100\n"
        "Uid:\t1001\t1001\t1001\t1001\n"
        "VmSize:\t   32768 kB\n"
        "VmRSS:\t    8192 kB\n"
        "Threads:\t4\n";

    std::istringstream iss(content);
    minimon::Process p;
    ASSERT_TRUE(minimon::ProcReader::parse_status_stream(iss, p));
    EXPECT_EQ(p.name, "python3");
    EXPECT_EQ(p.state, 'R');
    EXPECT_EQ(p.uid, 1001u);
    EXPECT_EQ(p.vm_size_kb, 32768u);
    EXPECT_EQ(p.rss_kb, 8192u);
    EXPECT_DOUBLE_EQ(p.rss_mb(), 8.0);
    EXPECT_EQ(p.threads, 4);
}

TEST(ProcStatusTest, ParseKernelThreadStatusStream) {
    // Kernel threads do not have VmSize or VmRSS lines in /proc/[pid]/status
    std::string content =
        "Name:\tkworker/u16:0\n"
        "State:\tI (idle)\n"
        "Uid:\t0\t0\t0\t0\n"
        "Threads:\t1\n";

    std::istringstream iss(content);
    minimon::Process p;
    ASSERT_TRUE(minimon::ProcReader::parse_status_stream(iss, p));
    EXPECT_EQ(p.name, "kworker/u16:0");
    EXPECT_EQ(p.state, 'I');
    EXPECT_EQ(p.uid, 0u);
    EXPECT_EQ(p.vm_size_kb, 0u);
    EXPECT_EQ(p.rss_kb, 0u);
    EXPECT_DOUBLE_EQ(p.rss_mb(), 0.0);
    EXPECT_EQ(p.threads, 1);
}

TEST(ProcCmdlineTest, ParseNulSeparatedBuffer) {
    // Note: Use separate string literal fragments to avoid compiler interpreting \010 as octal \b
    const char raw[] = "minimon\0" "--search\0" "chrome\0" "--top\0" "10\0";
    size_t len = sizeof(raw) - 1; // 33 bytes including embedded nulls
    std::string parsed = minimon::ProcReader::parse_cmdline_buffer(raw, len);
    EXPECT_EQ(parsed, "minimon --search chrome --top 10");
}

TEST(ProcCmdlineTest, ParseEmptyBuffer) {
    std::string parsed = minimon::ProcReader::parse_cmdline_buffer(nullptr, 0);
    EXPECT_EQ(parsed, "");

    const char empty_buf[] = "\0";
    EXPECT_EQ(minimon::ProcReader::parse_cmdline_buffer(empty_buf, 1), "");
}

TEST(ProcStatusCmdlineTest, ReadFromFixtures) {
    std::string fixture_dir = std::string(TEST_FIXTURES_DIR) + "/proc_status_cmdline";
    minimon::ProcReader reader(fixture_dir);

    // Test PID 500 (standard bash process)
    minimon::Process p500;
    ASSERT_TRUE(reader.read_status(500, p500));
    EXPECT_EQ(p500.name, "bash");
    EXPECT_EQ(p500.state, 'S');
    EXPECT_EQ(p500.uid, 1000u);
    EXPECT_EQ(p500.vm_size_kb, 16384u);
    EXPECT_EQ(p500.rss_kb, 4096u);
    EXPECT_DOUBLE_EQ(p500.rss_mb(), 4.0);
    EXPECT_EQ(p500.threads, 1);

    ASSERT_TRUE(reader.read_cmdline(500, p500));
    EXPECT_EQ(p500.cmdline, "/bin/bash -l");

    // Test PID 2 (kernel thread kthreadd)
    minimon::Process p2;
    ASSERT_TRUE(reader.read_status(2, p2));
    EXPECT_EQ(p2.name, "kthreadd");
    EXPECT_EQ(p2.uid, 0u);
    EXPECT_EQ(p2.vm_size_kb, 0u);
    EXPECT_EQ(p2.rss_kb, 0u);
    EXPECT_EQ(p2.threads, 2);

    ASSERT_TRUE(reader.read_cmdline(2, p2));
    EXPECT_EQ(p2.cmdline, "");
}

TEST(UserLookupTest, RootUserLookup) {
    minimon::ProcReader reader;
    // UID 0 is universally defined as "root" on UNIX systems
    std::string root_user = reader.get_username(0);
    EXPECT_EQ(root_user, "root");
}

TEST(UserLookupTest, UnknownUidFallback) {
    minimon::ProcReader reader;
    // Very high UID that does not correspond to any registered user
    uid_t non_existent_uid = 9999999;
    std::string fallback = reader.get_username(non_existent_uid);
    EXPECT_EQ(fallback, "9999999");
}

TEST(UserLookupTest, CachingBehavior) {
    minimon::ProcReader reader;
    std::string u1 = reader.get_username(0);
    std::string u2 = reader.get_username(0);
    EXPECT_EQ(u1, u2);
    EXPECT_EQ(u1, "root");

    reader.clear_user_cache();
    std::string u3 = reader.get_username(0);
    EXPECT_EQ(u3, "root");
}

TEST(UserLookupTest, StatusIntegration) {
    std::string fixture_dir = std::string(TEST_FIXTURES_DIR) + "/proc_status_cmdline";
    minimon::ProcReader reader(fixture_dir);

    // PID 2 in our fixture has Uid: 0, which should resolve to "root"
    minimon::Process p2;
    ASSERT_TRUE(reader.read_status(2, p2));
    EXPECT_EQ(p2.uid, 0u);
    EXPECT_EQ(p2.user, "root");
}

TEST(SystemMeminfoTest, ParseMeminfoStream) {
    std::string content =
        "MemTotal:       16384000 kB\n"
        "MemFree:         4096000 kB\n"
        "MemAvailable:   12288000 kB\n"
        "Buffers:          512000 kB\n"
        "Cached:          7680000 kB\n"
        "SwapTotal:       2048000 kB\n"
        "SwapFree:        1024000 kB\n";

    std::istringstream iss(content);
    minimon::SystemMemory mem;
    ASSERT_TRUE(minimon::ProcReader::parse_meminfo_stream(iss, mem));
    EXPECT_EQ(mem.total_kb, 16384000u);
    EXPECT_EQ(mem.free_kb, 4096000u);
    EXPECT_EQ(mem.available_kb, 12288000u);
    // used = total - available = 16384000 - 12288000 = 4096000 kB
    EXPECT_EQ(mem.used_kb(), 4096000u);
    EXPECT_DOUBLE_EQ(mem.used_percent(), 25.0);
    EXPECT_DOUBLE_EQ(mem.total_mb(), 16000.0);
    EXPECT_DOUBLE_EQ(mem.used_mb(), 4000.0);
    EXPECT_EQ(mem.swap_total_kb, 2048000u);
    EXPECT_EQ(mem.swap_free_kb, 1024000u);
}

TEST(SystemCpuTest, ParseCpuStatStream) {
    std::string content =
        "cpu  1000 200 500 8000 100 50 50 100 0 0\n"
        "cpu0 500 100 250 4000 50 25 25 50 0 0\n";

    std::istringstream iss(content);
    minimon::SystemCpuTimes cpu;
    ASSERT_TRUE(minimon::ProcReader::parse_stat_cpu_stream(iss, cpu));
    EXPECT_EQ(cpu.user, 1000u);
    EXPECT_EQ(cpu.nice, 200u);
    EXPECT_EQ(cpu.system, 500u);
    EXPECT_EQ(cpu.idle, 8000u);
    EXPECT_EQ(cpu.iowait, 100u);
    EXPECT_EQ(cpu.irq, 50u);
    EXPECT_EQ(cpu.softirq, 50u);
    EXPECT_EQ(cpu.steal, 100u);
    // total = 1000 + 200 + 500 + 8000 + 100 + 50 + 50 + 100 = 10000
    EXPECT_EQ(cpu.total_time(), 10000u);
    // idle = idle + iowait = 8000 + 100 = 8100
    EXPECT_EQ(cpu.idle_time(), 8100u);
    // active = 10000 - 8100 = 1900
    EXPECT_EQ(cpu.active_time(), 1900u);
}

TEST(SystemFixturesTest, ReadFromDiskFixtures) {
    std::string fixture_dir = std::string(TEST_FIXTURES_DIR) + "/proc_system";
    minimon::ProcReader reader(fixture_dir);

    minimon::SystemMemory mem;
    ASSERT_TRUE(reader.read_meminfo(mem));
    EXPECT_EQ(mem.total_kb, 16384000u);
    EXPECT_EQ(mem.used_kb(), 4096000u);
    EXPECT_DOUBLE_EQ(mem.used_percent(), 25.0);

    minimon::SystemCpuTimes cpu;
    ASSERT_TRUE(reader.read_system_cpu(cpu));
    EXPECT_EQ(cpu.total_time(), 10000u);
    EXPECT_EQ(cpu.active_time(), 1900u);
}

TEST(ProcFullScanTest, ReadAllProcessesFromFixture) {
    std::string fixture_dir = std::string(TEST_FIXTURES_DIR) + "/proc_full";
    minimon::ProcReader reader(fixture_dir);
    minimon::CpuTracker tracker(100);

    std::vector<minimon::Process> procs = reader.read_all_processes(tracker);

    // Directory had 1, 2, 100, and 999 (where 999 vanished and has no stat file)
    ASSERT_EQ(procs.size(), 3u);

    // Verify Process 1 (systemd)
    EXPECT_EQ(procs[0].pid, 1);
    EXPECT_EQ(procs[0].name, "systemd");
    EXPECT_EQ(procs[0].user, "root");
    EXPECT_EQ(procs[0].rss_kb, 8192u);
    EXPECT_DOUBLE_EQ(procs[0].mem_percent, (8192.0 / 16384000.0) * 100.0);
    EXPECT_EQ(procs[0].cmdline, "/sbin/init");
    EXPECT_DOUBLE_EQ(procs[0].cpu_percent, 0.0); // Initial sample

    // Verify Process 2 (kthreadd - kernel thread)
    EXPECT_EQ(procs[1].pid, 2);
    EXPECT_EQ(procs[1].name, "kthreadd");
    EXPECT_EQ(procs[1].user, "root");
    EXPECT_EQ(procs[1].rss_kb, 0u);
    EXPECT_DOUBLE_EQ(procs[1].mem_percent, 0.0);
    EXPECT_EQ(procs[1].cmdline, "[kthreadd]"); // Bracketed fallback for empty cmdline

    // Verify Process 100 (bash)
    EXPECT_EQ(procs[2].pid, 100);
    EXPECT_EQ(procs[2].name, "bash");
    EXPECT_EQ(procs[2].rss_kb, 4096u);
    EXPECT_DOUBLE_EQ(procs[2].mem_percent, (4096.0 / 16384000.0) * 100.0);
    EXPECT_EQ(procs[2].cmdline, "/bin/bash");

    // Verify that tracker has recorded 3 processes
    EXPECT_EQ(tracker.tracked_count(), 3u);
}
