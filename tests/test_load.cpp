#include <chrono>
#include <iterator>
#include <filesystem>
#include <thread>

#include "rtlb/load.hpp"
#include "test.hpp"

using namespace rtlb;

namespace {
std::size_t entries(const std::filesystem::path& dir) {
    return static_cast<std::size_t>(std::distance(std::filesystem::directory_iterator(dir),
                                                  std::filesystem::directory_iterator()));
}
}  // namespace

TEST(no_loads_starts_no_threads) {
    const Config c;
    const LoadSet set(c);
    CHECK_EQ(set.thread_count(), 0u);
}

TEST(all_load_kinds_start_and_stop_cleanly) {
    Config c;
    c.loads = {LoadKind::Cpu, LoadKind::Memory, LoadKind::Io};
    c.load_threads = 2;
    c.mem_mib = 4;
    const auto dir = std::filesystem::temp_directory_path() / "rtlb-test-load";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    c.io_dir = dir.string();
    {
        const LoadSet set(c);
        CHECK_EQ(set.thread_count(), 6u);
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        CHECK_EQ(entries(dir), 2u);
    }
    CHECK_EQ(entries(dir), 0u);
    std::filesystem::remove_all(dir);
}

TEST(unwritable_io_dir_throws) {
    Config c;
    c.loads = {LoadKind::Io};
    c.io_dir = "/nonexistent/rtlb-dir";
    CHECK_THROWS(LoadSet(c));
}

RTLB_TEST_MAIN()
