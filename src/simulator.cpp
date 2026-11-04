#include "rtlb/simulator.hpp"

#include <cmath>
#include <random>

namespace rtlb {
namespace {

struct Profile {
    double base_us;
    double jitter_us;
    double spike_probability;
    double spike_us;
};

Profile profile_for(Policy policy, std::size_t load_kinds) {
    Profile profile{};
    switch (policy) {
        case Policy::Fifo: profile = {6.0, 1.5, 1e-4, 35.0}; break;
        case Policy::Rr: profile = {7.0, 1.8, 1.5e-4, 40.0}; break;
        case Policy::Other: profile = {45.0, 20.0, 2e-3, 500.0}; break;
    }
    const double load = 1.0 + (0.6 * static_cast<double>(load_kinds));
    profile.jitter_us *= load;
    profile.spike_probability *= load;
    profile.spike_us *= load;
    return profile;
}

}  // namespace

std::uint64_t simulate_thread(const Config& config, unsigned thread_index, Policy policy,
                              Histogram& histogram) {
    constexpr std::uint64_t kGolden = 0x9E3779B97F4A7C15ULL;
    std::mt19937_64 rng(config.seed ^ (kGolden * (static_cast<std::uint64_t>(thread_index) + 1)));
    const auto uniform = [&rng] {
        return static_cast<double>(rng() >> 11) * (1.0 / 9007199254740992.0);
    };
    const auto exponential = [&uniform] { return -std::log(1.0 - uniform()); };

    const Profile profile = profile_for(policy, config.loads.size());
    const std::int64_t interval_ns = config.interval_us * 1000;
    const std::uint64_t samples = samples_per_thread(config);
    std::uint64_t overruns = 0;
    for (std::uint64_t i = 0; i < samples; ++i) {
        double latency_us = profile.base_us + (profile.jitter_us * exponential());
        if (uniform() < profile.spike_probability) {
            latency_us += profile.spike_us * (0.5 + exponential());
        }
        const auto latency_ns = static_cast<std::int64_t>(std::llround(latency_us * 1000.0));
        histogram.record(latency_ns);
        if (latency_ns >= interval_ns) {
            ++overruns;
        }
    }
    return overruns;
}

}  // namespace rtlb
