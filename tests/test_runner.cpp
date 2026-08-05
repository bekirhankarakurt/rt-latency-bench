#include <sstream>

#include "rtlb/runner.hpp"
#include "test.hpp"

using namespace rtlb;

namespace {
Config live_config() {
    Config c;
    c.policy = Policy::Other;
    c.interval_us = 1000;
    c.duration_s = 0.2;
    return c;
}
}  // namespace

TEST(simulated_run_fills_every_thread) {
    Config c;
    c.simulate = true;
    c.threads = 3;
    c.cpus = {0, 1};
    c.duration_s = 1.0;
    const Report r = run(c);
    CHECK(r.mode == Mode::Simulate);
    CHECK_EQ(r.threads.size(), 3u);
    for (const ThreadResult& t : r.threads) CHECK_EQ(t.hist.count(), 1000u);
    CHECK_EQ(r.threads[2].cpu, 0);
    CHECK_EQ(merged_histogram(r).count(), 3000u);
}

TEST(invalid_config_throws) {
    Config c;
    c.threads = 0;
    CHECK_THROWS(run(c));
}

TEST(missing_replay_file_throws) {
    Config c;
    c.replay_path = "/nonexistent/rtlb.txt";
    CHECK_THROWS(run(c));
}

TEST(live_run_measures_real_wakeups) {
    const Report r = run(live_config());
    CHECK(r.mode == Mode::Live);
    CHECK_EQ(r.threads.size(), 1u);
    CHECK(r.threads[0].hist.count() > 100u);
    CHECK(r.threads[0].hist.max_ns() > 0);
    CHECK(r.threads[0].policy == Policy::Other);
}

TEST(live_realtime_request_degrades_gracefully) {
    Config c = live_config();
    c.policy = Policy::Fifo;
    c.priority = 10;
    c.mlock = true;
    c.cpus = {0};
    c.loads = {LoadKind::Cpu};
    const Report r = run(c);
    const ThreadResult& t = r.threads[0];
    CHECK(t.hist.count() > 0);
    // Either the request was honoured, or a fallback was reported.
    CHECK(t.policy == Policy::Fifo || !t.notes.empty());
}

TEST(live_multi_thread_run_reports_each_thread) {
    Config c = live_config();
    c.threads = 2;
    const Report r = run(c);
    CHECK_EQ(r.threads.size(), 2u);
    CHECK(r.threads[1].hist.count() > 100u);
}

RTLB_TEST_MAIN()
