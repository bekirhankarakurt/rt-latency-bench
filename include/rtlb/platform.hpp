#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "rtlb/config.hpp"

// Thin, thread-safe wrappers over the Linux timing and scheduling syscalls.
// Failures are reported as values so callers can degrade gracefully when the
// process lacks privileges.
namespace rtlb::platform {

struct Outcome {
    bool ok = true;
    std::string message;  // reason when !ok
};

[[nodiscard]] std::int64_t now_ns() noexcept;
// Sleeps until an absolute CLOCK_MONOTONIC time (TIMER_ABSTIME), retrying on EINTR.
void sleep_until_ns(std::int64_t deadline_ns) noexcept;

[[nodiscard]] std::vector<int> allowed_cpus();
// Restricts the calling thread to the given CPUs.
[[nodiscard]] Outcome pin_to_cpus(const std::vector<int>& cpus);
// Sets the scheduling policy of the calling thread.
[[nodiscard]] Outcome set_policy(Policy policy, int priority);
[[nodiscard]] Policy current_policy() noexcept;
// mlockall(MCL_CURRENT | MCL_FUTURE) for the whole process.
[[nodiscard]] Outcome lock_memory();

}  // namespace rtlb::platform
