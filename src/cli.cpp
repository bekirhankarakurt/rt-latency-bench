#include "rtlb/cli.hpp"

#include <array>
#include <optional>
#include <string_view>
#include <utility>

#include "rtlb/parse.hpp"

namespace rtlb {
namespace {

struct Alias {
    std::string_view short_name;
    std::string_view long_name;
};

constexpr std::array<Alias, 8> kAliases{{
    {"-t", "--threads"}, {"-i", "--interval"}, {"-d", "--duration"}, {"-p", "--policy"},
    {"-P", "--priority"}, {"-a", "--affinity"}, {"-l", "--load"}, {"-m", "--mlock"},
}};

constexpr std::array<std::string_view, 6> kFlags{
    "--mlock", "--histogram", "--simulate", "--help", "--version", "-h"};

bool is_flag(std::string_view key) {
    for (const std::string_view flag : kFlags) {
        if (flag == key) return true;
    }
    return false;
}

template <class T>
std::string number(std::string_view key, std::string_view value, T& out) {
    return parse_integer(value, out) ? std::string{}
                                     : "invalid number for " + std::string(key) + ": " + std::string(value);
}

std::string apply(std::string_view key, std::string_view value, ParseResult& result) {
    Config& c = result.config;
    if (key == "--mlock") { c.mlock = true; return {}; }
    if (key == "--histogram") { c.histogram = true; return {}; }
    if (key == "--simulate") { c.simulate = true; return {}; }
    if (key == "--help" || key == "-h") { result.command = Command::Help; return {}; }
    if (key == "--version" || key == "-V") { result.command = Command::Version; return {}; }

    if (key == "--threads") return number(key, value, c.threads);
    if (key == "--interval") return number(key, value, c.interval_us);
    if (key == "--priority") return number(key, value, c.priority);
    if (key == "--load-threads") return number(key, value, c.load_threads);
    if (key == "--mem-mib") return number(key, value, c.mem_mib);
    if (key == "--bucket-us") return number(key, value, c.bucket_us);
    if (key == "--max-us") return number(key, value, c.max_us);
    if (key == "--seed") return number(key, value, c.seed);
    if (key == "--duration") {
        return parse_double(value, c.duration_s) ? std::string{}
                                                 : "invalid number for --duration: " + std::string(value);
    }
    if (key == "--threshold-us") {
        double limit = 0.0;
        if (!parse_double(value, limit)) return "invalid number for --threshold-us: " + std::string(value);
        c.threshold_us = limit;
        return {};
    }
    if (key == "--policy" || key == "--vs") {
        const auto policy = parse_policy(value);
        if (!policy) return "unknown policy '" + std::string(value) + "' (use fifo, rr or other)";
        (key == "--policy" ? c.policy : c.vs) = *policy;
        return {};
    }
    if (key == "--affinity") {
        auto cpus = parse_cpu_list(value);
        if (!cpus) return "invalid CPU list '" + std::string(value) + "' (example: 0,2-3)";
        c.cpus = std::move(*cpus);
        return {};
    }
    if (key == "--load") {
        auto loads = parse_loads(value);
        if (!loads) return "invalid load list '" + std::string(value) + "' (use cpu,mem,io or none)";
        c.loads = std::move(*loads);
        return {};
    }
    if (key == "--io-dir") { c.io_dir = std::string(value); return {}; }
    if (key == "--csv") { c.csv_path = std::string(value); return {}; }
    if (key == "--json") { c.json_path = std::string(value); return {}; }
    if (key == "--replay") { c.replay_path = std::string(value); return {}; }
    return "unknown option: " + std::string(key);
}

}  // namespace

ParseResult parse_args(const std::vector<std::string>& args) {
    ParseResult result;
    std::size_t i = 0;
    if (!args.empty() && (args[0] == "run" || args[0] == "compare")) {
        result.command = args[0] == "compare" ? Command::Compare : Command::Run;
        i = 1;
    }
    for (; i < args.size(); ++i) {
        std::string key = args[i];
        std::optional<std::string> inline_value;
        const std::size_t equals = key.find('=');
        if (key.rfind("--", 0) == 0 && equals != std::string::npos) {
            inline_value = key.substr(equals + 1);
            key.resize(equals);
        }
        for (const Alias& alias : kAliases) {
            if (key == alias.short_name) key = std::string(alias.long_name);
        }
        std::string value;
        if (!is_flag(key) && key != "-V") {
            if (inline_value) {
                value = *inline_value;
            } else if (i + 1 < args.size()) {
                value = args[++i];
            } else {
                result.error = "missing value for " + key;
                return result;
            }
        }
        result.error = apply(key, value, result);
        if (!result.error.empty()) return result;
        if (result.command == Command::Help || result.command == Command::Version) return result;
    }

    if (result.command == Command::Compare &&
        (!result.config.csv_path.empty() || !result.config.json_path.empty())) {
        result.error = "compare does not support --csv or --json";
        return result;
    }
    result.error = validate(result.config);
    return result;
}

std::string usage() {
    return R"(rtlb - Linux real-time latency lab (cyclictest-style)

Usage:
  rtlb [run] [options]        measure wake-up latency
  rtlb compare [options]      run twice (--policy vs --vs) and print the difference

Timing:
  -t, --threads N         measurement threads (default 1)
  -i, --interval US       period in microseconds (default 1000)
  -d, --duration S        run time in seconds (default 10)
Scheduling:
  -p, --policy P          fifo | rr | other (default fifo)
  -P, --priority N        real-time priority 1-99 (default 80)
  -a, --affinity LIST     CPUs, e.g. 0,2-3; thread i uses entry i mod N
  -m, --mlock             mlockall(MCL_CURRENT|MCL_FUTURE)
Load:
  -l, --load LIST         cpu,mem,io or none (default none)
      --load-threads N    threads per load kind (default 1)
      --mem-mib N         memory load buffer size in MiB (default 64)
      --io-dir DIR        directory for the IO load file (default temp dir)
Histogram:
      --bucket-us N       bucket width in microseconds (default 1)
      --max-us N          histogram range in microseconds (default 10000)
      --histogram         print an ASCII histogram
Output:
      --csv FILE          write per-thread summary as CSV
      --json FILE         write full report (with histogram) as JSON
      --threshold-us N    exit with status 3 if max latency exceeds N
Simulation:
      --simulate          synthetic latencies, no sleeping or privileges
      --seed N            simulation seed (default 1)
      --replay FILE       replay recorded latencies (microseconds, one per line)
Compare:
      --vs P              policy for the second run (default other)
Other:
  -h, --help              show this help
  -V, --version           show version

Exit status: 0 ok, 1 runtime error, 2 usage error, 3 threshold exceeded.
)";
}

}  // namespace rtlb
