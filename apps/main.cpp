#include <csignal>
#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "rtlb/cli.hpp"
#include "rtlb/report.hpp"
#include "rtlb/runner.hpp"

namespace {

constexpr int kExitRuntimeError = 1;
constexpr int kExitUsage = 2;
constexpr int kExitThreshold = 3;

extern "C" void on_interrupt(int) { rtlb::request_stop(); }

void install_interrupt_handler() {
    struct sigaction action {};
    action.sa_handler = on_interrupt;
    sigemptyset(&action.sa_mask);
    sigaction(SIGINT, &action, nullptr);
    sigaction(SIGTERM, &action, nullptr);
}

template <class Writer>
void export_to(const std::string& path, Writer write, const rtlb::Report& report) {
    if (path.empty()) return;
    std::ofstream file(path);
    if (!file) throw std::runtime_error("cannot write " + path);
    write(file, report);
    std::cout << "wrote " << path << "\n";
}

int run_command(const rtlb::Config& config) {
    const rtlb::Report report = rtlb::run(config);
    rtlb::print_report(std::cout, report);
    if (config.histogram) rtlb::print_histogram(std::cout, report);
    export_to(config.csv_path, rtlb::write_csv, report);
    export_to(config.json_path, rtlb::write_json, report);

    if (config.threshold_us) {
        const double max_us = static_cast<double>(rtlb::merged_histogram(report).max_ns()) / 1000.0;
        if (max_us > *config.threshold_us) {
            std::cerr << "FAIL: max latency " << max_us << " us exceeds threshold "
                      << *config.threshold_us << " us\n";
            return kExitThreshold;
        }
    }
    return 0;
}

int compare_command(const rtlb::Config& config) {
    rtlb::Config second = config;
    second.policy = config.vs;
    const rtlb::Report a = rtlb::run(config);
    const rtlb::Report b = rtlb::run(second);
    rtlb::print_comparison(std::cout, a, b);
    if (config.histogram) {
        rtlb::print_histogram(std::cout, a);
        rtlb::print_histogram(std::cout, b);
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    const std::vector<std::string> args(argv + 1, argv + argc);
    const rtlb::ParseResult parsed = rtlb::parse_args(args);
    if (!parsed.error.empty()) {
        std::cerr << "rtlb: " << parsed.error << "\nTry 'rtlb --help'.\n";
        return kExitUsage;
    }
    try {
        switch (parsed.command) {
            case rtlb::Command::Help:
                std::cout << rtlb::usage();
                return 0;
            case rtlb::Command::Version:
                std::cout << "rtlb " << RTLB_VERSION << "\n";
                return 0;
            case rtlb::Command::Compare:
                install_interrupt_handler();
                return compare_command(parsed.config);
            case rtlb::Command::Run:
                install_interrupt_handler();
                return run_command(parsed.config);
        }
    } catch (const std::exception& error) {
        std::cerr << "rtlb: error: " << error.what() << "\n";
        return kExitRuntimeError;
    }
    return 0;
}
