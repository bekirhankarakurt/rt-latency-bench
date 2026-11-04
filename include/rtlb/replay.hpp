#pragma once

#include <cstdint>
#include <istream>
#include <string>
#include <vector>

#include "rtlb/histogram.hpp"

namespace rtlb {

// Reads latencies in microseconds, one per line. Blank lines and text after '#'
// are ignored. Returns nanoseconds; throws std::runtime_error on malformed input.
[[nodiscard]] std::vector<std::int64_t> parse_replay(std::istream& input);
[[nodiscard]] std::vector<std::int64_t> load_replay_file(const std::string& path);

// Feeds every `threads`-th sample, starting at `thread_index`, into the histogram.
// Returns the number of overruns (latencies of at least one interval).
[[nodiscard]] std::uint64_t replay_thread(const std::vector<std::int64_t>& samples_ns,
                                          unsigned thread_index, unsigned threads,
                                          std::int64_t interval_ns, Histogram& histogram);

}  // namespace rtlb
