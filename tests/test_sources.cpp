#include <sstream>

#include "rtlb/replay.hpp"
#include "rtlb/simulator.hpp"
#include "test.hpp"

using namespace rtlb;

namespace {
Config sim_config(Policy policy) {
    Config c;
    c.simulate = true;
    c.policy = policy;
    c.duration_s = 5.0;
    return c;
}

Summary simulate(const Config& c, unsigned thread = 0) {
    Histogram h(c.bucket_us * 1000, c.max_us * 1000);
    (void)simulate_thread(c, thread, c.policy, h);
    return summarize(h);
}
}  // namespace

TEST(simulation_produces_expected_sample_count) {
    const Config c = sim_config(Policy::Fifo);
    CHECK_EQ(simulate(c).samples, samples_per_thread(c));
}

TEST(simulation_is_deterministic_per_seed) {
    Config c = sim_config(Policy::Fifo);
    const Summary a = simulate(c);
    const Summary b = simulate(c);
    CHECK_EQ(a.max_ns, b.max_ns);
    CHECK_EQ(a.p99_ns, b.p99_ns);
    c.seed = 2;
    CHECK(simulate(c).avg_ns != a.avg_ns);
    CHECK(simulate(c, 1).avg_ns != simulate(c, 0).avg_ns);
}

TEST(realtime_policy_beats_other_in_simulation) {
    const Summary fifo = simulate(sim_config(Policy::Fifo));
    const Summary other = simulate(sim_config(Policy::Other));
    CHECK(fifo.p99_ns < other.p99_ns);
    CHECK(fifo.avg_ns < other.avg_ns);
}

TEST(load_increases_simulated_latency) {
    Config idle = sim_config(Policy::Fifo);
    Config loaded = idle;
    loaded.loads = {LoadKind::Cpu, LoadKind::Memory, LoadKind::Io};
    CHECK(simulate(loaded).avg_ns > simulate(idle).avg_ns);
}

TEST(simulation_counts_overruns) {
    Config c = sim_config(Policy::Other);
    c.interval_us = 20;  // base latency alone exceeds the period
    Histogram h(1000, 10'000'000);
    CHECK(simulate_thread(c, 0, c.policy, h) > 0);
}

TEST(replay_parses_comments_and_blanks) {
    std::istringstream in("# header\n10\n\n 12.5 # inline\n3\r\n");
    const auto samples = parse_replay(in);
    CHECK_EQ(samples.size(), 3u);
    CHECK_EQ(samples[0], 10000);
    CHECK_EQ(samples[1], 12500);
    CHECK_EQ(samples[2], 3000);
}

TEST(replay_rejects_malformed_lines) {
    std::istringstream bad("5\nabc\n");
    CHECK_THROWS(parse_replay(bad));
    std::istringstream negative("-1\n");
    CHECK_THROWS(parse_replay(negative));
    CHECK_THROWS(load_replay_file("/nonexistent/rtlb.txt"));
}

TEST(replay_distributes_round_robin_and_counts_overruns) {
    const std::vector<std::int64_t> samples{1000, 2000, 3000, 4000, 5000};
    Histogram a(1000, 100000);
    Histogram b(1000, 100000);
    const auto overruns_a = replay_thread(samples, 0, 2, 3000, a);
    const auto overruns_b = replay_thread(samples, 1, 2, 3000, b);
    CHECK_EQ(a.count(), 3u);
    CHECK_EQ(b.count(), 2u);
    CHECK_EQ(overruns_a, 2u);  // 3000 and 5000
    CHECK_EQ(overruns_b, 1u);  // 4000
}

RTLB_TEST_MAIN()
