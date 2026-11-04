#pragma once

#include <ostream>

#include "rtlb/runner.hpp"

namespace rtlb {

/// Per-thread table plus an aggregate row and any warnings.
void print_report(std::ostream& out, const Report& report);
/// Logarithmic-bar ASCII histogram of the merged samples.
void print_histogram(std::ostream& out, const Report& report);
/// Side-by-side aggregate metrics of two runs.
void print_comparison(std::ostream& out, const Report& a, const Report& b);
/// One row per thread plus an `all` row; values in microseconds.
void write_csv(std::ostream& out, const Report& report);
/// Configuration, per-thread and aggregate statistics, and the non-empty histogram bins.
void write_json(std::ostream& out, const Report& report);

}  // namespace rtlb
