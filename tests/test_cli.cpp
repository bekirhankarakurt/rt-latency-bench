#include "rtlb/cli.hpp"

#include <string>
#include <vector>

#include "test.hpp"

using namespace rtlb;

namespace {
ParseResult parse(std::vector<std::string> args) { return parse_args(args); }
}  // namespace

TEST(cpu_list_expands_ranges) {
    const auto cpus = parse_cpu_list("0,2-4");
    CHECK(cpus.has_value());
    CHECK((*cpus == std::vector<int>{0, 2, 3, 4}));
    CHECK(!parse_cpu_list("").has_value());
    CHECK(!parse_cpu_list("3-1").has_value());
    CHECK(!parse_cpu_list("1,").has_value());
    CHECK(!parse_cpu_list("a").has_value());
    CHECK(!parse_cpu_list("-1").has_value());
}

TEST(policy_and_load_parsing) {
    CHECK(parse_policy("rr") == Policy::Rr);
    CHECK(!parse_policy("batch").has_value());
    const auto loads = parse_loads("cpu,io,cpu");
    CHECK(loads.has_value());
    CHECK_EQ(loads->size(), 2u);
    CHECK(parse_loads("none")->empty());
    CHECK(!parse_loads("gpu").has_value());
    CHECK(!parse_loads("cpu,").has_value());
}

TEST(defaults_are_valid) {
    const ParseResult r = parse({});
    CHECK(r.error.empty());
    CHECK(r.command == Command::Run);
    CHECK(r.config.policy == Policy::Fifo);
    CHECK_EQ(samples_per_thread(r.config), 10000u);
}

TEST(long_short_and_inline_forms) {
    const ParseResult r = parse({"-t", "4", "--interval=500", "-p", "rr", "-P", "50",
                                 "-a", "0-1", "-l", "cpu,mem", "--mlock", "--duration", "2.5"});
    CHECK(r.error.empty());
    CHECK_EQ(r.config.threads, 4u);
    CHECK_EQ(r.config.interval_us, 500);
    CHECK(r.config.policy == Policy::Rr);
    CHECK_EQ(r.config.priority, 50);
    CHECK_EQ(r.config.cpus.size(), 2u);
    CHECK_EQ(r.config.loads.size(), 2u);
    CHECK(r.config.mlock);
    CHECK_NEAR(r.config.duration_s, 2.5, 1e-12);
}

TEST(subcommands_and_help) {
    CHECK(parse({"compare", "--vs", "fifo"}).command == Command::Compare);
    CHECK(parse({"run"}).command == Command::Run);
    CHECK(parse({"--help"}).command == Command::Help);
    CHECK(parse({"-V"}).command == Command::Version);
    CHECK(usage().find("compare") != std::string::npos);
}

TEST(invalid_input_reports_errors) {
    CHECK(!parse({"--bogus"}).error.empty());
    CHECK(!parse({"--threads"}).error.empty());
    CHECK(!parse({"--threads", "x"}).error.empty());
    CHECK(!parse({"--threads", "0"}).error.empty());
    CHECK(!parse({"--policy", "edf"}).error.empty());
    CHECK(!parse({"--priority", "100"}).error.empty());
    CHECK(!parse({"--interval", "5"}).error.empty());
    CHECK(!parse({"--duration", "0.0001", "--interval", "1000"}).error.empty());
    CHECK(!parse({"--simulate", "--replay", "x.txt"}).error.empty());
    CHECK(!parse({"compare", "--csv", "x.csv"}).error.empty());
    CHECK(!parse({"--max-us", "10", "--bucket-us", "20"}).error.empty());
}

TEST(other_policy_ignores_priority_range) {
    CHECK(parse({"--policy", "other", "--priority", "0"}).error.empty());
}

TEST(mode_selection) {
    CHECK(mode_of(parse({}).config) == Mode::Live);
    CHECK(mode_of(parse({"--simulate"}).config) == Mode::Simulate);
    CHECK(mode_of(parse({"--replay", "f"}).config) == Mode::Replay);
}

RTLB_TEST_MAIN()
