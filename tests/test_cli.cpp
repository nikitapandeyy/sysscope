#include <gtest/gtest.h>
#include "cli.hpp"
#include <sstream>

TEST(CliTest, DefaultConfig) {
    minimon::CliConfig config;
    const char* argv[] = {"minimon"};
    ASSERT_TRUE(minimon::parse_cli_args(1, argv, config));
    EXPECT_FALSE(config.once);
    EXPECT_FALSE(config.json);
    EXPECT_FALSE(config.show_help);
    EXPECT_DOUBLE_EQ(config.interval, 1.0);
    EXPECT_EQ(config.top_n, 20u);
    EXPECT_EQ(config.sort_field, minimon::SortField::Cpu);
    EXPECT_EQ(config.sort_order, minimon::SortOrder::Descending);
    EXPECT_EQ(config.proc_root, "/proc");
}

TEST(CliTest, HelpFlag) {
    minimon::CliConfig config;
    const char* argv[] = {"minimon", "--help"};
    ASSERT_TRUE(minimon::parse_cli_args(2, argv, config));
    EXPECT_TRUE(config.show_help);

    std::ostringstream oss;
    minimon::print_help(oss);
    EXPECT_NE(oss.str().find("Usage: minimon [OPTIONS]"), std::string::npos);
}

TEST(CliTest, SearchAndFilterFlags) {
    minimon::CliConfig config;
    const char* argv[] = {
        "minimon",
        "--search", "chrome",
        "--user", "root",
        "--state", "S",
        "--pid", "1234",
        "--min-cpu", "10.5",
        "--min-mem", "256.0"
    };
    ASSERT_TRUE(minimon::parse_cli_args(13, argv, config));
    EXPECT_EQ(config.filter.name_search, "chrome");
    EXPECT_EQ(config.filter.user, "root");
    EXPECT_EQ(config.filter.state, 'S');
    ASSERT_TRUE(config.filter.pid.has_value());
    EXPECT_EQ(*config.filter.pid, 1234);
    EXPECT_DOUBLE_EQ(config.filter.min_cpu, 10.5);
    EXPECT_DOUBLE_EQ(config.filter.min_mem_mb, 256.0);
}

TEST(CliTest, SortAndDisplayFlags) {
    minimon::CliConfig config;
    const char* argv[] = {
        "minimon",
        "--once",
        "--json",
        "--sort", "mem",
        "--asc",
        "--top", "15",
        "--interval", "2.5"
    };
    ASSERT_TRUE(minimon::parse_cli_args(10, argv, config));
    EXPECT_TRUE(config.once);
    EXPECT_TRUE(config.json);
    EXPECT_EQ(config.sort_field, minimon::SortField::Memory);
    EXPECT_EQ(config.sort_order, minimon::SortOrder::Ascending);
    EXPECT_EQ(config.top_n, 15u);
    EXPECT_DOUBLE_EQ(config.interval, 2.5);
}

TEST(CliTest, MissingOrInvalidArguments) {
    minimon::CliConfig c1;
    const char* a1[] = {"minimon", "--search"}; // Missing search string
    EXPECT_FALSE(minimon::parse_cli_args(2, a1, c1));
    EXPECT_NE(c1.error_message.find("Missing argument"), std::string::npos);

    minimon::CliConfig c2;
    const char* a2[] = {"minimon", "--pid", "not_a_number"};
    EXPECT_FALSE(minimon::parse_cli_args(3, a2, c2));
    EXPECT_NE(c2.error_message.find("Invalid integer for PID"), std::string::npos);

    minimon::CliConfig c3;
    const char* a3[] = {"minimon", "--sort", "invalid_sort"};
    EXPECT_FALSE(minimon::parse_cli_args(3, a3, c3));
    EXPECT_NE(c3.error_message.find("Invalid sort field"), std::string::npos);

    minimon::CliConfig c4;
    const char* a4[] = {"minimon", "--interval", "-1.0"};
    EXPECT_FALSE(minimon::parse_cli_args(3, a4, c4));
    EXPECT_NE(c4.error_message.find("greater than 0"), std::string::npos);
}
