#include "rtlb/histogram.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace rtlb {

Histogram::Histogram(std::int64_t bucket_ns, std::int64_t max_ns) : bucket_ns_(bucket_ns) {
    if (bucket_ns <= 0 || max_ns < bucket_ns) {
        throw std::invalid_argument("histogram needs 0 < bucket <= max");
    }
    bins_.assign(static_cast<std::size_t>(max_ns / bucket_ns), 0);
}

void Histogram::record(std::int64_t latency_ns) noexcept {
    const std::int64_t value = latency_ns < 0 ? 0 : latency_ns;
    if (count_ == 0 || value < min_) {
        min_ = value;
    }
    max_ = std::max(max_, value);
    ++count_;
    sum_ += static_cast<double>(value);
    const auto index = static_cast<std::uint64_t>(value / bucket_ns_);
    if (index < bins_.size()) {
        ++bins_[static_cast<std::size_t>(index)];
    } else {
        ++overflow_;
    }
}

void Histogram::merge(const Histogram& other) {
    if (other.bucket_ns_ != bucket_ns_ || other.bins_.size() != bins_.size()) {
        throw std::invalid_argument("cannot merge histograms with different geometry");
    }
    if (other.count_ == 0) {
        return;
    }
    min_ = count_ == 0 ? other.min_ : std::min(min_, other.min_);
    max_ = std::max(max_, other.max_);
    count_ += other.count_;
    overflow_ += other.overflow_;
    sum_ += other.sum_;
    for (std::size_t i = 0; i < bins_.size(); ++i) {
        bins_[i] += other.bins_[i];
    }
}

double Histogram::mean_ns() const noexcept {
    return count_ == 0 ? 0.0 : sum_ / static_cast<double>(count_);
}

std::int64_t Histogram::percentile_ns(double pct) const noexcept {
    if (count_ == 0) {
        return 0;
    }
    const double clamped = std::min(std::max(pct, 0.0), 100.0);
    // The epsilon keeps products such as 99.9% of 10000 from rounding up a rank.
    auto rank = static_cast<std::uint64_t>(
        std::ceil((clamped / 100.0 * static_cast<double>(count_)) - 1e-9));
    rank = std::min(std::max<std::uint64_t>(rank, 1), count_);

    std::uint64_t cumulative = 0;
    for (std::size_t i = 0; i < bins_.size(); ++i) {
        cumulative += bins_[i];
        if (cumulative >= rank) {
            return std::min(static_cast<std::int64_t>(i + 1) * bucket_ns_, max_);
        }
    }
    return max_;
}

Summary summarize(const Histogram& histogram) noexcept {
    Summary s;
    s.samples = histogram.count();
    s.min_ns = histogram.min_ns();
    s.avg_ns = histogram.mean_ns();
    s.max_ns = histogram.max_ns();
    s.p50_ns = histogram.percentile_ns(50.0);
    s.p99_ns = histogram.percentile_ns(99.0);
    s.p999_ns = histogram.percentile_ns(99.9);
    return s;
}

}  // namespace rtlb
