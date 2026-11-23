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

TEST_F(FilterTest, ParseSortField) {
    EXPECT_EQ(minimon::parse_sort_field("cpu"), minimon::SortField::Cpu);
    EXPECT_EQ(minimon::parse_sort_field("CPU"), minimon::SortField::Cpu);
    EXPECT_EQ(minimon::parse_sort_field("mem"), minimon::SortField::Memory);
    EXPECT_EQ(minimon::parse_sort_field("memory"), minimon::SortField::Memory);
    EXPECT_EQ(minimon::parse_sort_field("rss"), minimon::SortField::Memory);
    EXPECT_EQ(minimon::parse_sort_field("pid"), minimon::SortField::Pid);
    EXPECT_EQ(minimon::parse_sort_field("name"), minimon::SortField::Name);
    EXPECT_EQ(minimon::parse_sort_field("comm"), minimon::SortField::Name);
    EXPECT_FALSE(minimon::parse_sort_field("unknown").has_value());
}

TEST_F(FilterTest, SortByCpuDescending) {
    auto sorted = procs;
    minimon::sort_processes(sorted, minimon::SortField::Cpu, minimon::SortOrder::Descending);
    ASSERT_EQ(sorted.size(), 4u);
    EXPECT_EQ(sorted[0].pid, 200); // 35.5%
    EXPECT_EQ(sorted[1].pid, 100); // 2.0%
    EXPECT_EQ(sorted[2].pid, 1);   // 0.5%
    EXPECT_EQ(sorted[3].pid, 300); // 0.0%
}

TEST_F(FilterTest, SortByCpuAscending) {
    auto sorted = procs;
    minimon::sort_processes(sorted, minimon::SortField::Cpu, minimon::SortOrder::Ascending);
    ASSERT_EQ(sorted.size(), 4u);
    EXPECT_EQ(sorted[0].pid, 300); // 0.0%
    EXPECT_EQ(sorted[1].pid, 1);   // 0.5%
    EXPECT_EQ(sorted[2].pid, 100); // 2.0%
    EXPECT_EQ(sorted[3].pid, 200); // 35.5%
}

TEST_F(FilterTest, SortByMemoryDescending) {
    auto sorted = procs;
    minimon::sort_processes(sorted, minimon::SortField::Memory, minimon::SortOrder::Descending);
    ASSERT_EQ(sorted.size(), 4u);
    EXPECT_EQ(sorted[0].pid, 200); // 512 MB
    EXPECT_EQ(sorted[1].pid, 100); // 50 MB
    EXPECT_EQ(sorted[2].pid, 1);   // 10 MB
    EXPECT_EQ(sorted[3].pid, 300); // 0 MB
}

TEST_F(FilterTest, SortByMemoryAscending) {
    auto sorted = procs;
    minimon::sort_processes(sorted, minimon::SortField::Memory, minimon::SortOrder::Ascending);
    ASSERT_EQ(sorted.size(), 4u);
    EXPECT_EQ(sorted[0].pid, 300); // 0 MB
    EXPECT_EQ(sorted[1].pid, 1);   // 10 MB
    EXPECT_EQ(sorted[2].pid, 100); // 50 MB
    EXPECT_EQ(sorted[3].pid, 200); // 512 MB
}

TEST_F(FilterTest, SortByPidAscendingAndDescending) {
    auto asc = procs;
    minimon::sort_processes(asc, minimon::SortField::Pid, minimon::SortOrder::Ascending);
    EXPECT_EQ(asc[0].pid, 1);
    EXPECT_EQ(asc[1].pid, 100);
    EXPECT_EQ(asc[2].pid, 200);
    EXPECT_EQ(asc[3].pid, 300);

    auto desc = procs;
    minimon::sort_processes(desc, minimon::SortField::Pid, minimon::SortOrder::Descending);
    EXPECT_EQ(desc[0].pid, 300);
    EXPECT_EQ(desc[1].pid, 200);
    EXPECT_EQ(desc[2].pid, 100);
    EXPECT_EQ(desc[3].pid, 1);
}

TEST_F(FilterTest, SortByNameAscending) {
    auto sorted = procs;
    minimon::sort_processes(sorted, minimon::SortField::Name, minimon::SortOrder::Ascending);
    EXPECT_EQ(sorted[0].name, "bash");
    EXPECT_EQ(sorted[1].name, "chrome");
    EXPECT_EQ(sorted[2].name, "systemd");
    EXPECT_EQ(sorted[3].name, "zombie_proc");
}
