#pragma once

#include <cstdint>
#include <vector>

namespace rtlb {

/// Fixed-geometry latency histogram. Recording never allocates, so it is safe
/// to call from the timing loop of a real-time thread.
class Histogram {
public:
    /// Buckets of width `bucket_ns` cover [0, max_ns); larger values are counted
    /// in the overflow bin. Throws std::invalid_argument on a bad geometry.
    Histogram(std::int64_t bucket_ns, std::int64_t max_ns);

    /// Negative latencies (early wake-ups) are clamped to zero.
    void record(std::int64_t latency_ns) noexcept;

    /// Adds another histogram with identical geometry; throws std::invalid_argument otherwise.
    void merge(const Histogram& other);

    [[nodiscard]] std::uint64_t count() const noexcept { return count_; }
    [[nodiscard]] std::int64_t min_ns() const noexcept { return count_ == 0 ? 0 : min_; }
    [[nodiscard]] std::int64_t max_ns() const noexcept { return max_; }
    [[nodiscard]] double mean_ns() const noexcept;

    /// Upper edge of the bucket holding the sample of rank ceil(pct/100 * count),
    /// clamped to the observed maximum. `pct` is in (0, 100].
    [[nodiscard]] std::int64_t percentile_ns(double pct) const noexcept;

    [[nodiscard]] std::int64_t bucket_ns() const noexcept { return bucket_ns_; }
    [[nodiscard]] const std::vector<std::uint64_t>& bins() const noexcept { return bins_; }
    [[nodiscard]] std::uint64_t overflow() const noexcept { return overflow_; }

private:
    std::int64_t bucket_ns_;
    std::vector<std::uint64_t> bins_;
    std::uint64_t overflow_ = 0;
    std::uint64_t count_ = 0;
    std::int64_t min_ = 0;
    std::int64_t max_ = 0;
    double sum_ = 0.0;
};

struct Summary {
    std::uint64_t samples = 0;
    std::int64_t min_ns = 0;
    double avg_ns = 0.0;
    std::int64_t max_ns = 0;
    std::int64_t p50_ns = 0;
    std::int64_t p99_ns = 0;
    std::int64_t p999_ns = 0;
};

[[nodiscard]] Summary summarize(const Histogram& histogram) noexcept;

}  // namespace rtlb
