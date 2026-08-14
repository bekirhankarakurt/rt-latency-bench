#pragma once

#include <cstdint>

#include "rtlb/config.hpp"
#include "rtlb/histogram.hpp"

namespace rtlb {

/// Deterministic synthetic latency source: an exponential jitter around a
/// per-policy base plus rare scheduling spikes, both scaled by the number of
/// load kinds. Runs in virtual time. Returns the number of overruns
/// (latencies of at least one interval).
[[nodiscard]] std::uint64_t simulate_thread(const Config& config, unsigned thread_index,
                                            Policy policy, Histogram& histogram);

}  // namespace rtlb
