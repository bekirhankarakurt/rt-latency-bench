#include "rtlb/histogram.hpp"

#include "test.hpp"

using rtlb::Histogram;

TEST(empty_histogram_reports_zeros) {
    const Histogram h(1000, 100000);
    CHECK_EQ(h.count(), 0u);
    CHECK_EQ(h.min_ns(), 0);
    CHECK_EQ(h.percentile_ns(99.0), 0);
    CHECK_NEAR(h.mean_ns(), 0.0, 1e-12);
}

TEST(percentiles_use_bucket_upper_edge) {
    Histogram h(1000, 1000000);
    for (std::int64_t i = 0; i < 100; ++i) {
        h.record(i * 1000 + 500);
    }
    CHECK_EQ(h.count(), 100u);
    CHECK_EQ(h.min_ns(), 500);
    CHECK_EQ(h.max_ns(), 99500);
    CHECK_EQ(h.percentile_ns(50.0), 50000);
    CHECK_EQ(h.percentile_ns(99.0), 99000);
    CHECK_EQ(h.percentile_ns(100.0), 99500);
    CHECK_NEAR(h.mean_ns(), 50000.0, 1e-6);
}

TEST(p999_rank_is_not_rounded_up) {
    Histogram h(1000, 1000000);
    for (int i = 0; i < 9990; ++i) {
        h.record(1500);
    }
    for (int i = 0; i < 10; ++i) {
        h.record(500500);
    }
    CHECK_EQ(h.percentile_ns(99.9), 2000);
    CHECK_EQ(h.percentile_ns(99.95), 500500);
}

TEST(negative_latency_is_clamped) {
    Histogram h(1000, 10000);
    h.record(-5);
    CHECK_EQ(h.min_ns(), 0);
    CHECK_EQ(h.bins()[0], 1u);
}

TEST(values_beyond_range_go_to_overflow) {
    Histogram h(1000, 10000);
    h.record(10000);
    h.record(2500000);
    CHECK_EQ(h.overflow(), 2u);
    CHECK_EQ(h.max_ns(), 2500000);
    CHECK_EQ(h.percentile_ns(99.0), 2500000);
}

TEST(merge_combines_counts_and_extremes) {
    Histogram a(1000, 10000);
    Histogram b(1000, 10000);
    a.record(3500);
    b.record(900);
    b.record(7500);
    a.merge(b);
    CHECK_EQ(a.count(), 3u);
    CHECK_EQ(a.min_ns(), 900);
    CHECK_EQ(a.max_ns(), 7500);
    CHECK_NEAR(a.mean_ns(), 3966.6666666, 1e-3);
}

TEST(merge_into_empty_adopts_minimum) {
    Histogram a(1000, 10000);
    Histogram b(1000, 10000);
    b.record(4200);
    a.merge(b);
    CHECK_EQ(a.min_ns(), 4200);
}

TEST(invalid_geometry_throws) {
    CHECK_THROWS(Histogram(0, 100));
    CHECK_THROWS(Histogram(100, 50));
    Histogram a(1000, 10000);
    const Histogram other(500, 10000);
    CHECK_THROWS(a.merge(other));
}

TEST(summary_matches_queries) {
    Histogram h(1000, 1000000);
    for (std::int64_t i = 1; i <= 1000; ++i) {
        h.record(i * 1000 - 500);
    }
    const rtlb::Summary s = rtlb::summarize(h);
    CHECK_EQ(s.samples, 1000u);
    CHECK_EQ(s.p50_ns, 500000);
    CHECK_EQ(s.p99_ns, 990000);
    CHECK_EQ(s.p999_ns, 999000);
    CHECK_EQ(s.max_ns, 999500);
}

RTLB_TEST_MAIN()
