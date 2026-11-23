#include <gtest/gtest.h>
#include "process.hpp"

TEST(ProcessTest, DefaultInitialization) {
    minimon::Process p;
    EXPECT_EQ(p.pid, 0);
    EXPECT_EQ(p.ppid, 0);
    EXPECT_EQ(p.state, '?');
    EXPECT_DOUBLE_EQ(p.cpu_percent, 0.0);
    EXPECT_EQ(p.rss_kb, 0u);
    EXPECT_DOUBLE_EQ(p.rss_mb(), 0.0);
}

TEST(ProcessTest, RssMbConversion) {
    minimon::Process p;
    p.rss_kb = 2048; // 2 MB
    EXPECT_DOUBLE_EQ(p.rss_mb(), 2.0);

    p.rss_kb = 512;  // 0.5 MB
    EXPECT_DOUBLE_EQ(p.rss_mb(), 0.5);
}
