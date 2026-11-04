#include "rtlb/replay.hpp"

#include <cmath>
#include <fstream>
#include <stdexcept>

#include "rtlb/parse.hpp"

namespace rtlb {

std::vector<std::int64_t> parse_replay(std::istream& input) {
    std::vector<std::int64_t> samples;
    std::string line;
    for (std::size_t number = 1; std::getline(input, line); ++number) {
        const std::size_t comment = line.find('#');
        if (comment != std::string::npos) {
            line.resize(comment);
        }
        const std::size_t first = line.find_first_not_of(" \t\r");
        if (first == std::string::npos) {
            continue;
        }
        const std::size_t last = line.find_last_not_of(" \t\r");
        double microseconds = 0.0;
        if (!parse_double(std::string_view(line).substr(first, last - first + 1), microseconds) ||
            microseconds < 0.0) {
            throw std::runtime_error("replay line " + std::to_string(number) +
                                     ": expected a non-negative number of microseconds");
        }
        samples.push_back(static_cast<std::int64_t>(std::llround(microseconds * 1000.0)));
    }
    return samples;
}

std::vector<std::int64_t> load_replay_file(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("cannot open replay file: " + path);
    }
    std::vector<std::int64_t> samples = parse_replay(file);
    if (samples.empty()) {
        throw std::runtime_error("replay file has no samples: " + path);
    }
    return samples;
}

std::uint64_t replay_thread(const std::vector<std::int64_t>& samples_ns, unsigned thread_index,
                            unsigned threads, std::int64_t interval_ns, Histogram& histogram) {
    std::uint64_t overruns = 0;
    for (std::size_t i = thread_index; i < samples_ns.size(); i += threads) {
        histogram.record(samples_ns[i]);
        if (samples_ns[i] >= interval_ns) {
            ++overruns;
        }
    }
    return overruns;
}

}  // namespace rtlb
