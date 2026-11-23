#include <gtest/gtest.h>
#include "filter.hpp"

class FilterTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup synthetic process list
        minimon::Process p1;
        p1.pid = 1;
        p1.name = "systemd";
        p1.user = "root";
        p1.state = 'S';
        p1.cpu_percent = 0.5;
        p1.rss_kb = 10240; // 10 MB
        p1.cmdline = "/sbin/init";
        procs.push_back(p1);

        minimon::Process p2;
        p2.pid = 100;
        p2.name = "bash";
        p2.user = "nikita";
        p2.state = 'S';
        p2.cpu_percent = 2.0;
        p2.rss_kb = 51200; // 50 MB
        p2.cmdline = "/bin/bash --login";
        procs.push_back(p2);

        minimon::Process p3;
        p3.pid = 200;
        p3.name = "chrome";
        p3.user = "nikita";
        p3.state = 'R';
        p3.cpu_percent = 35.5;
        p3.rss_kb = 524288; // 512 MB
        p3.cmdline = "/opt/google/chrome/chrome --no-sandbox";
        procs.push_back(p3);

        minimon::Process p4;
        p4.pid = 300;
        p4.name = "zombie_proc";
        p4.user = "nikita";
        p4.state = 'Z';
        p4.cpu_percent = 0.0;
        p4.rss_kb = 0; // 0 MB
        p4.cmdline = "[zombie_proc]";
        procs.push_back(p4);
    }

    std::vector<minimon::Process> procs;
};

TEST_F(FilterTest, StringHelpers) {
    EXPECT_TRUE(minimon::case_insensitive_contains("Hello World", "world"));
    EXPECT_TRUE(minimon::case_insensitive_contains("Chrome", "CHRO"));
    EXPECT_FALSE(minimon::case_insensitive_contains("systemd", "firefox"));

    EXPECT_TRUE(minimon::case_insensitive_equals("Root", "root"));
    EXPECT_FALSE(minimon::case_insensitive_equals("root", "daemon"));
}

TEST_F(FilterTest, EmptyFilterReturnsAll) {
    minimon::FilterOptions opts;
    auto res = minimon::apply_filter(procs, opts);
    EXPECT_EQ(res.size(), 4u);
}

TEST_F(FilterTest, FilterByNameSubstring) {
    minimon::FilterOptions opts;
    opts.name_search = "CHROME";
    auto res = minimon::apply_filter(procs, opts);
    ASSERT_EQ(res.size(), 1u);
    EXPECT_EQ(res[0].pid, 200);

    opts.name_search = "sh"; // Matches "bash" in name
    res = minimon::apply_filter(procs, opts);
    ASSERT_EQ(res.size(), 1u);
    EXPECT_EQ(res[0].pid, 100);
}

TEST_F(FilterTest, FilterByCmdlineSubstring) {
    minimon::FilterOptions opts;
    opts.name_search = "--login"; // Only in bash cmdline
    auto res = minimon::apply_filter(procs, opts);
    ASSERT_EQ(res.size(), 1u);
    EXPECT_EQ(res[0].pid, 100);
}

TEST_F(FilterTest, FilterByPid) {
    minimon::FilterOptions opts;
    opts.pid = 100;
    auto res = minimon::apply_filter(procs, opts);
    ASSERT_EQ(res.size(), 1u);
    EXPECT_EQ(res[0].pid, 100);

    opts.pid = 9999; // Non-existent
    res = minimon::apply_filter(procs, opts);
    EXPECT_TRUE(res.empty());
}

TEST_F(FilterTest, FilterByUser) {
    minimon::FilterOptions opts;
    opts.user = "root";
    auto res = minimon::apply_filter(procs, opts);
    ASSERT_EQ(res.size(), 1u);
    EXPECT_EQ(res[0].pid, 1);

    opts.user = "NIKITA";
    res = minimon::apply_filter(procs, opts);
    EXPECT_EQ(res.size(), 3u);
}

TEST_F(FilterTest, FilterByState) {
    minimon::FilterOptions opts;
    opts.state = 'z'; // Case-insensitive 'z' -> 'Z'
    auto res = minimon::apply_filter(procs, opts);
    ASSERT_EQ(res.size(), 1u);
    EXPECT_EQ(res[0].pid, 300);

    opts.state = 'R';
    res = minimon::apply_filter(procs, opts);
    ASSERT_EQ(res.size(), 1u);
    EXPECT_EQ(res[0].pid, 200);
}

TEST_F(FilterTest, FilterByMinCpu) {
    minimon::FilterOptions opts;
    opts.min_cpu = 10.0;
    auto res = minimon::apply_filter(procs, opts);
    ASSERT_EQ(res.size(), 1u);
    EXPECT_EQ(res[0].pid, 200); // 35.5%

    opts.min_cpu = 50.0;
    res = minimon::apply_filter(procs, opts);
    EXPECT_TRUE(res.empty());
}

TEST_F(FilterTest, FilterByMinMemory) {
    minimon::FilterOptions opts;
    opts.min_mem_mb = 100.0; // 100 MB
    auto res = minimon::apply_filter(procs, opts);
    ASSERT_EQ(res.size(), 1u);
    EXPECT_EQ(res[0].pid, 200); // 512 MB
}

TEST_F(FilterTest, CombinedFilter) {
    minimon::FilterOptions opts;
    opts.user = "nikita";
    opts.state = 'S';
    opts.min_cpu = 1.0;
    auto res = minimon::apply_filter(procs, opts);
    ASSERT_EQ(res.size(), 1u);
    EXPECT_EQ(res[0].pid, 100);
}
