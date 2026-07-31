#include "rtlb/platform.hpp"

#include "test.hpp"

using namespace rtlb;

TEST(clock_is_monotonic) {
    const auto a = platform::now_ns();
    const auto b = platform::now_ns();
    CHECK(b >= a);
}

TEST(absolute_sleep_reaches_deadline) {
    const auto deadline = platform::now_ns() + 2'000'000;
    platform::sleep_until_ns(deadline);
    CHECK(platform::now_ns() >= deadline);
}

TEST(sleep_in_the_past_returns_immediately) {
    const auto start = platform::now_ns();
    platform::sleep_until_ns(start - 1'000'000);
    CHECK(platform::now_ns() - start < 500'000'000);
}

TEST(allowed_cpus_is_not_empty) {
    CHECK(!platform::allowed_cpus().empty());
}

TEST(pin_rejects_unavailable_cpu) {
    const auto outcome = platform::pin_to_cpus({4095});
    CHECK(!outcome.ok);
    CHECK(!outcome.message.empty());
}

TEST(unprivileged_policies_are_consistent) {
    // SCHED_OTHER must always be settable; real-time may be refused without privileges.
    CHECK(platform::set_policy(Policy::Other, 0).ok);
    CHECK(platform::current_policy() == Policy::Other);
    const auto fifo = platform::set_policy(Policy::Fifo, 1);
    CHECK(fifo.ok == (platform::current_policy() == Policy::Fifo));
    CHECK(fifo.ok || !fifo.message.empty());
}

RTLB_TEST_MAIN()
