#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace rtlb {

enum class Policy { Fifo, Rr, Other };
enum class LoadKind { Cpu, Memory, Io };
enum class Mode { Live, Simulate, Replay };

struct Config {
    unsigned threads = 1;
    std::int64_t interval_us = 1000;
    double duration_s = 10.0;
    Policy policy = Policy::Fifo;
    int priority = 80;
    std::vector<int> cpus;  // thread i is pinned to cpus[i % size]; empty = no pinning
    bool mlock = false;

    std::vector<LoadKind> loads;
    unsigned load_threads = 1;  // per load kind
    unsigned mem_mib = 64;
    std::string io_dir;  // empty = system temp directory

    std::int64_t bucket_us = 1;
    std::int64_t max_us = 10000;

    bool histogram = false;
    std::string csv_path;
    std::string json_path;

    bool simulate = false;
    std::uint64_t seed = 1;
    std::string replay_path;

    Policy vs = Policy::Other;  // comparison target for `compare`
    std::optional<double> threshold_us;
};

[[nodiscard]] const char* to_string(Policy policy) noexcept;
[[nodiscard]] const char* to_string(LoadKind kind) noexcept;
[[nodiscard]] const char* to_string(Mode mode) noexcept;

[[nodiscard]] std::optional<Policy> parse_policy(std::string_view text);
// Comma-separated subset of cpu,mem,io, or "none".
[[nodiscard]] std::optional<std::vector<LoadKind>> parse_loads(std::string_view text);
// CPU list such as "0,2-3".
[[nodiscard]] std::optional<std::vector<int>> parse_cpu_list(std::string_view text);

[[nodiscard]] Mode mode_of(const Config& config) noexcept;
[[nodiscard]] std::uint64_t samples_per_thread(const Config& config) noexcept;
// Returns an empty string when valid, otherwise a human-readable reason.
[[nodiscard]] std::string validate(const Config& config);

}  // namespace rtlb
