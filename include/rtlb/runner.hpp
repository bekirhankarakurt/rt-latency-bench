#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "rtlb/config.hpp"
#include "rtlb/histogram.hpp"

namespace rtlb {

struct ThreadResult {
    ThreadResult(unsigned thread_id, Histogram histogram)
        : id(thread_id), hist(std::move(histogram)) {}

    unsigned id;
    int cpu = -1;  ///< -1 = not pinned
    Policy policy = Policy::Other;  ///< effective policy
    int priority = 0;               // effective priority
    Histogram hist;
    std::uint64_t overruns = 0;
    std::vector<std::string> notes;  ///< degraded-mode messages
};

struct Report {
    Config config;
    Mode mode = Mode::Live;
    std::vector<ThreadResult> threads;
    std::vector<std::string> warnings;
};

/// Runs the benchmark described by `config` (live, simulated or replayed).
/// Throws std::runtime_error on unrecoverable errors such as an unreadable replay file.
[[nodiscard]] Report run(const Config& config);

/// Asks a running benchmark to finish early; async-signal-safe.
void request_stop() noexcept;

[[nodiscard]] Histogram merged_histogram(const Report& report);
[[nodiscard]] std::uint64_t total_overruns(const Report& report) noexcept;

}  // namespace rtlb
