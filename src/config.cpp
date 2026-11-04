#include "rtlb/config.hpp"

#include <cmath>

#include "rtlb/parse.hpp"

namespace rtlb {

const char* to_string(Policy policy) noexcept {
    switch (policy) {
        case Policy::Fifo: return "fifo";
        case Policy::Rr: return "rr";
        case Policy::Other: return "other";
    }
    return "other";
}

const char* to_string(LoadKind kind) noexcept {
    switch (kind) {
        case LoadKind::Cpu: return "cpu";
        case LoadKind::Memory: return "mem";
        case LoadKind::Io: return "io";
    }
    return "cpu";
}

const char* to_string(Mode mode) noexcept {
    switch (mode) {
        case Mode::Live: return "live";
        case Mode::Simulate: return "simulate";
        case Mode::Replay: return "replay";
    }
    return "live";
}

std::optional<Policy> parse_policy(std::string_view text) {
    if (text == "fifo") return Policy::Fifo;
    if (text == "rr") return Policy::Rr;
    if (text == "other") return Policy::Other;
    return std::nullopt;
}

std::optional<std::vector<LoadKind>> parse_loads(std::string_view text) {
    std::vector<LoadKind> loads;
    if (text == "none") {
        return loads;
    }
    std::size_t pos = 0;
    while (pos <= text.size()) {
        std::size_t comma = text.find(',', pos);
        if (comma == std::string_view::npos) {
            comma = text.size();
        }
        const std::string_view token = text.substr(pos, comma - pos);
        LoadKind kind{};
        if (token == "cpu") kind = LoadKind::Cpu;
        else if (token == "mem") kind = LoadKind::Memory;
        else if (token == "io") kind = LoadKind::Io;
        else return std::nullopt;
        bool duplicate = false;
        for (const LoadKind existing : loads) {
            duplicate = duplicate || existing == kind;
        }
        if (!duplicate) {
            loads.push_back(kind);
        }
        pos = comma + 1;
    }
    return loads;
}

std::optional<std::vector<int>> parse_cpu_list(std::string_view text) {
    constexpr int kMaxCpu = 4096;
    std::vector<int> cpus;
    std::size_t pos = 0;
    while (pos <= text.size()) {
        std::size_t comma = text.find(',', pos);
        if (comma == std::string_view::npos) {
            comma = text.size();
        }
        const std::string_view token = text.substr(pos, comma - pos);
        const std::size_t dash = token.find('-');
        int low = 0;
        int high = 0;
        if (dash == std::string_view::npos) {
            if (!parse_integer(token, low)) return std::nullopt;
            high = low;
        } else if (!parse_integer(token.substr(0, dash), low) ||
                   !parse_integer(token.substr(dash + 1), high)) {
            return std::nullopt;
        }
        if (low < 0 || high < low || high >= kMaxCpu) {
            return std::nullopt;
        }
        for (int cpu = low; cpu <= high; ++cpu) {
            cpus.push_back(cpu);
        }
        pos = comma + 1;
    }
    return cpus;
}

Mode mode_of(const Config& config) noexcept {
    if (!config.replay_path.empty()) return Mode::Replay;
    return config.simulate ? Mode::Simulate : Mode::Live;
}

std::uint64_t samples_per_thread(const Config& config) noexcept {
    const double samples = config.duration_s * 1e6 / static_cast<double>(config.interval_us);
    return static_cast<std::uint64_t>(std::llround(samples));
}

std::string validate(const Config& c) {
    if (c.threads < 1 || c.threads > 256) return "--threads must be between 1 and 256";
    if (c.interval_us < 20 || c.interval_us > 10'000'000) {
        return "--interval must be between 20 and 10000000 us";
    }
    if (!(c.duration_s > 0.0) || c.duration_s > 86400.0) {
        return "--duration must be in (0, 86400] seconds";
    }
    if (c.policy != Policy::Other && (c.priority < 1 || c.priority > 99)) {
        return "--priority must be between 1 and 99";
    }
    if (c.bucket_us < 1) return "--bucket-us must be at least 1";
    if (c.max_us < c.bucket_us || c.max_us / c.bucket_us > 1'000'000) {
        return "--max-us must be >= --bucket-us and span at most 1000000 buckets";
    }
    if (c.load_threads < 1 || c.load_threads > 64) return "--load-threads must be between 1 and 64";
    if (c.mem_mib < 1 || c.mem_mib > 16384) return "--mem-mib must be between 1 and 16384";
    if (c.simulate && !c.replay_path.empty()) return "--simulate and --replay are mutually exclusive";
    if (c.threshold_us && !(*c.threshold_us > 0.0)) return "--threshold-us must be positive";
    if (samples_per_thread(c) == 0) return "duration is shorter than one interval";
    return {};
}

}  // namespace rtlb
