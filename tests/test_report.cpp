#include <algorithm>
#include <sstream>

#include "rtlb/report.hpp"
#include "test.hpp"

using namespace rtlb;

namespace {
Report simulated(Policy policy, unsigned threads = 2) {
    Config c;
    c.simulate = true;
    c.policy = policy;
    c.threads = threads;
    c.duration_s = 2.0;
    c.loads = {LoadKind::Cpu};
    return run(c);
}
std::size_t count(const std::string& text, char c) {
    return static_cast<std::size_t>(std::count(text.begin(), text.end(), c));
}
}  // namespace

TEST(table_lists_threads_aggregate_and_mode) {
    std::ostringstream out;
    print_report(out, simulated(Policy::Fifo));
    const std::string text = out.str();
    CHECK(text.find("mode=simulate") != std::string::npos);
    CHECK(text.find("p99.9_us") != std::string::npos);
    CHECK(text.find("all") != std::string::npos);
    CHECK(text.find("fifo/80") != std::string::npos);
}

TEST(table_shows_warnings) {
    Report r = simulated(Policy::Fifo, 1);
    r.warnings.push_back("memory not locked: test");
    std::ostringstream out;
    print_report(out, r);
    CHECK(out.str().find("warning: memory not locked: test") != std::string::npos);
}

TEST(csv_has_header_thread_rows_and_total) {
    std::ostringstream out;
    write_csv(out, simulated(Policy::Rr, 3));
    const std::string text = out.str();
    CHECK_EQ(count(text, '\n'), 5u);
    CHECK(text.rfind("thread,cpu,policy,samples,min_us", 0) == 0);
    CHECK(text.find("\nall,-1,rr/80,6000,") != std::string::npos);
}

TEST(json_is_balanced_and_complete) {
    Report r = simulated(Policy::Fifo);
    r.warnings.push_back("quote \" and \\ slash");
    std::ostringstream out;
    write_json(out, r);
    const std::string text = out.str();
    CHECK_EQ(count(text, '{'), count(text, '}'));
    CHECK_EQ(count(text, '['), count(text, ']'));
    CHECK(text.find("\"mode\": \"simulate\"") != std::string::npos);
    CHECK(text.find("\"p999_us\"") != std::string::npos);
    CHECK(text.find("\"bins\": [[") != std::string::npos);
    CHECK(text.find("quote \\\" and \\\\ slash") != std::string::npos);
}

TEST(ascii_histogram_draws_bars) {
    std::ostringstream out;
    print_histogram(out, simulated(Policy::Other));
    CHECK(out.str().find('#') != std::string::npos);
    CHECK(out.str().find(" us ") != std::string::npos);
}

TEST(comparison_shows_every_metric) {
    std::ostringstream out;
    print_comparison(out, simulated(Policy::Fifo), simulated(Policy::Other));
    const std::string text = out.str();
    for (const char* metric : {"min", "avg", "p50", "p99 ", "p99.9", "max", "overruns"}) {
        CHECK(text.find(metric) != std::string::npos);
    }
    CHECK(text.find("A = fifo/80") != std::string::npos);
    CHECK(text.find("B = other") != std::string::npos);
}

RTLB_TEST_MAIN()
