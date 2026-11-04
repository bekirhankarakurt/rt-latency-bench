#include "rtlb/platform.hpp"

#include <sched.h>
#include <sys/mman.h>
#include <cerrno>
#include <ctime>
#include <system_error>

namespace rtlb::platform {
namespace {

constexpr std::int64_t kNsPerSec = 1'000'000'000;

std::string errno_text(int error) { return std::generic_category().message(error); }

int native_policy(Policy policy) noexcept {
    switch (policy) {
        case Policy::Fifo: return SCHED_FIFO;
        case Policy::Rr: return SCHED_RR;
        case Policy::Other: return SCHED_OTHER;
    }
    return SCHED_OTHER;
}

}  // namespace

std::int64_t now_ns() noexcept {
    timespec ts{};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (static_cast<std::int64_t>(ts.tv_sec) * kNsPerSec) + static_cast<std::int64_t>(ts.tv_nsec);
}

void sleep_until_ns(std::int64_t deadline_ns) noexcept {
    timespec ts{};
    ts.tv_sec = static_cast<time_t>(deadline_ns / kNsPerSec);
    ts.tv_nsec = static_cast<long>(deadline_ns % kNsPerSec);
    while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, nullptr) == EINTR) {
    }
}

std::vector<int> allowed_cpus() {
    std::vector<int> cpus;
    cpu_set_t set;
    CPU_ZERO(&set);
    if (sched_getaffinity(0, sizeof(set), &set) == 0) {
        for (int cpu = 0; cpu < CPU_SETSIZE; ++cpu) {
            if (CPU_ISSET(cpu, &set)) {
                cpus.push_back(cpu);
            }
        }
    }
    return cpus;
}

Outcome pin_to_cpus(const std::vector<int>& cpus) {
    const std::vector<int> allowed = allowed_cpus();
    cpu_set_t set;
    CPU_ZERO(&set);
    for (const int cpu : cpus) {
        bool usable = false;
        for (const int candidate : allowed) {
            usable = usable || candidate == cpu;
        }
        if (!usable) {
            return {false, "cpu " + std::to_string(cpu) + " is not available to this process"};
        }
        CPU_SET(cpu, &set);
    }
    if (sched_setaffinity(0, sizeof(set), &set) != 0) {
        return {false, "sched_setaffinity: " + errno_text(errno)};
    }
    return {};
}

Outcome set_policy(Policy policy, int priority) {
    sched_param param{};
    param.sched_priority = policy == Policy::Other ? 0 : priority;
    if (sched_setscheduler(0, native_policy(policy), &param) != 0) {
        return {false, std::string("sched_setscheduler(") + to_string(policy) + ", " +
                           std::to_string(param.sched_priority) + "): " + errno_text(errno)};
    }
    return {};
}

Policy current_policy() noexcept {
    switch (sched_getscheduler(0)) {
        case SCHED_FIFO: return Policy::Fifo;
        case SCHED_RR: return Policy::Rr;
        default: return Policy::Other;
    }
}

Outcome lock_memory() {
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
        const int error = errno;
        munlockall();  // a failed call may still leave MCL_FUTURE set
        return {false, "mlockall: " + errno_text(error)};
    }
    return {};
}

}  // namespace rtlb::platform
