#include <gtest/gtest.h>
#include "proc_reader.hpp"

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
