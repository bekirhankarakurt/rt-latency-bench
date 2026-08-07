#include "rtlb/runner.hpp"

#include <atomic>
#include <functional>
#include <stdexcept>
#include <thread>
#include <utility>

#include "rtlb/load.hpp"
#include "rtlb/platform.hpp"
#include "rtlb/replay.hpp"
#include "rtlb/simulator.hpp"

namespace rtlb {
namespace {

static_assert(std::atomic<bool>::is_always_lock_free, "stop flag must be signal-safe");
std::atomic<bool> g_stop{false};

// Gives workers time to apply scheduling settings before the first deadline.
constexpr std::int64_t kStartDelayNs = 200'000'000;

struct ThreadGroup {
    std::vector<std::thread> threads;
    ~ThreadGroup() {
        for (std::thread& thread : threads) {
            if (thread.joinable()) thread.join();
        }
    }
};

int cpu_for(const Config& config, unsigned thread_id) {
    return config.cpus.empty() ? -1 : config.cpus[thread_id % config.cpus.size()];
}

void live_worker(const Config& config, ThreadResult& result, std::int64_t start_ns) {
    if (const int cpu = cpu_for(config, result.id); cpu >= 0) {
        const platform::Outcome pinned = platform::pin_to_cpus({cpu});
        if (pinned.ok) {
            result.cpu = cpu;
        } else {
            result.notes.push_back("not pinned: " + pinned.message);
        }
    }
    const platform::Outcome scheduled = platform::set_policy(config.policy, config.priority);
    if (!scheduled.ok) {
        result.notes.push_back(scheduled.message + "; running as SCHED_OTHER");
    }
    result.policy = platform::current_policy();
    result.priority = result.policy == Policy::Other ? 0 : config.priority;

    const std::int64_t interval_ns = config.interval_us * 1000;
    const std::uint64_t samples = samples_per_thread(config);
    std::int64_t next = start_ns + interval_ns;
    for (std::uint64_t i = 0; i < samples && !g_stop.load(std::memory_order_relaxed); ++i) {
        platform::sleep_until_ns(next);
        const std::int64_t woke = platform::now_ns();
        result.hist.record(woke - next);
        next += interval_ns;
        while (next <= woke) {  // missed deadlines: skip them and count the overruns
            next += interval_ns;
            ++result.overruns;
        }
    }
}

void run_live(const Config& config, Report& report) {
    const LoadSet load(config);
    const std::int64_t start_ns = platform::now_ns() + kStartDelayNs;
    ThreadGroup group;
    for (ThreadResult& result : report.threads) {
        group.threads.emplace_back(live_worker, std::cref(config), std::ref(result), start_ns);
    }
    // Locking after the threads exist keeps a low RLIMIT_MEMLOCK from failing thread creation.
    if (config.mlock) {
        const platform::Outcome locked = platform::lock_memory();
        if (!locked.ok) {
            report.warnings.push_back("memory not locked: " + locked.message);
        }
    }
    for (std::thread& thread : group.threads) {
        thread.join();
    }
}

void run_offline(const Config& config, Report& report) {
    std::vector<std::int64_t> recorded;
    if (report.mode == Mode::Replay) {
        recorded = load_replay_file(config.replay_path);
    }
    for (ThreadResult& result : report.threads) {
        const int cpu = cpu_for(config, result.id);
        result.cpu = cpu;
        result.policy = config.policy;
        result.priority = config.policy == Policy::Other ? 0 : config.priority;
        result.overruns =
            report.mode == Mode::Replay
                ? replay_thread(recorded, result.id, config.threads, config.interval_us * 1000,
                                result.hist)
                : simulate_thread(config, result.id, config.policy, result.hist);
    }
}

}  // namespace

Report run(const Config& config) {
    if (const std::string problem = validate(config); !problem.empty()) {
        throw std::runtime_error(problem);
    }
    Report report;
    report.config = config;
    report.mode = mode_of(config);
    report.threads.reserve(config.threads);
    for (unsigned id = 0; id < config.threads; ++id) {
        report.threads.emplace_back(id, Histogram(config.bucket_us * 1000, config.max_us * 1000));
    }

    if (report.mode == Mode::Live) {
        run_live(config, report);
    } else {
        run_offline(config, report);
    }

    for (const ThreadResult& result : report.threads) {
        for (const std::string& note : result.notes) {
            report.warnings.push_back("thread " + std::to_string(result.id) + ": " + note);
        }
    }
    return report;
}

void request_stop() noexcept { g_stop.store(true, std::memory_order_relaxed); }

Histogram merged_histogram(const Report& report) {
    Histogram total(report.config.bucket_us * 1000, report.config.max_us * 1000);
    for (const ThreadResult& result : report.threads) {
        total.merge(result.hist);
    }
    return total;
}

std::uint64_t total_overruns(const Report& report) noexcept {
    std::uint64_t sum = 0;
    for (const ThreadResult& result : report.threads) {
        sum += result.overruns;
    }
    return sum;
}

}  // namespace rtlb
