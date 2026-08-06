#include "rtlb/report.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <iomanip>
#include <string>

namespace rtlb {
namespace {

std::string fixed(double value, int digits) {
    std::array<char, 48> buffer{};
    std::snprintf(buffer.data(), buffer.size(), "%.*f", digits, value);
    return buffer.data();
}

std::string us1(double ns) { return fixed(ns / 1000.0, 1); }
std::string us3(double ns) { return fixed(ns / 1000.0, 3); }

std::string describe_policy(Policy policy, int priority) {
    return policy == Policy::Other ? std::string("other")
                                   : std::string(to_string(policy)) + "/" + std::to_string(priority);
}

std::string load_text(const Config& config) {
    if (config.loads.empty()) return "none";
    std::string text;
    for (const LoadKind kind : config.loads) {
        text += (text.empty() ? "" : ",") + std::string(to_string(kind));
    }
    return text;
}

std::string json_escape(const std::string& text) {
    std::string out;
    for (const char c : text) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    std::array<char, 8> escape{};
                    std::snprintf(escape.data(), escape.size(), "\\u%04x", static_cast<unsigned>(c));
                    out += escape.data();
                } else {
                    out += c;
                }
        }
    }
    return out;
}

struct Row {
    std::string thread, cpu, policy;
    Summary stats;
    std::uint64_t overruns;
};

std::vector<Row> rows_of(const Report& report) {
    std::vector<Row> rows;
    for (const ThreadResult& t : report.threads) {
        rows.push_back({std::to_string(t.id), t.cpu < 0 ? "-1" : std::to_string(t.cpu),
                        describe_policy(t.policy, t.priority), summarize(t.hist), t.overruns});
    }
    const ThreadResult& first = report.threads.front();
    rows.push_back({"all", "-1", describe_policy(first.policy, first.priority),
                    summarize(merged_histogram(report)), total_overruns(report)});
    return rows;
}

void json_stats(std::ostream& out, const Summary& s, std::uint64_t overruns) {
    out << "\"samples\": " << s.samples << ", \"min_us\": " << us3(static_cast<double>(s.min_ns))
        << ", \"avg_us\": " << us3(s.avg_ns) << ", \"max_us\": " << us3(static_cast<double>(s.max_ns))
        << ", \"p50_us\": " << us3(static_cast<double>(s.p50_ns))
        << ", \"p99_us\": " << us3(static_cast<double>(s.p99_ns))
        << ", \"p999_us\": " << us3(static_cast<double>(s.p999_ns)) << ", \"overruns\": " << overruns;
}

}  // namespace

void print_report(std::ostream& out, const Report& report) {
    const Config& c = report.config;
    out << "rt-latency-bench " << RTLB_VERSION << " | mode=" << to_string(report.mode)
        << " | policy=" << describe_policy(c.policy, c.priority) << " | interval=" << c.interval_us
        << " us | duration=" << fixed(c.duration_s, 1) << " s | load=" << load_text(c) << "\n\n";

    static constexpr int kWidths[] = {6, 4, 9, 9, 9, 9, 9, 9, 9, 9};
    const char* const heads[] = {"thread", "cpu", "policy", "samples", "min_us", "avg_us",
                                 "max_us", "p50_us", "p99_us", "p99.9_us"};
    const auto cell = [&out](int index, const std::string& text) {
        out << (index == 2 ? std::left : std::right) << std::setw(kWidths[index]) << text << "  ";
    };
    for (int i = 0; i < 10; ++i) cell(i, heads[i]);
    out << std::right << std::setw(8) << "overruns" << "\n";
    for (const Row& row : rows_of(report)) {
        const Summary& s = row.stats;
        cell(0, row.thread);
        cell(1, row.cpu == "-1" ? "-" : row.cpu);
        cell(2, row.policy);
        cell(3, std::to_string(s.samples));
        cell(4, us1(static_cast<double>(s.min_ns)));
        cell(5, us1(s.avg_ns));
        cell(6, us1(static_cast<double>(s.max_ns)));
        cell(7, us1(static_cast<double>(s.p50_ns)));
        cell(8, us1(static_cast<double>(s.p99_ns)));
        cell(9, us1(static_cast<double>(s.p999_ns)));
        out << std::right << std::setw(8) << row.overruns << "\n";
    }
    for (const std::string& warning : report.warnings) {
        out << "warning: " << warning << "\n";
    }
}

void print_histogram(std::ostream& out, const Report& report) {
    constexpr std::array<double, 13> kEdgesUs{1, 2, 5, 10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000};
    constexpr int kBarWidth = 40;
    const Histogram total = merged_histogram(report);
    std::array<std::uint64_t, kEdgesUs.size() + 1> rows{};
    const auto row_of = [&kEdgesUs](double low_us) {
        return static_cast<std::size_t>(std::upper_bound(kEdgesUs.begin(), kEdgesUs.end(), low_us) -
                                        kEdgesUs.begin());
    };
    for (std::size_t i = 0; i < total.bins().size(); ++i) {
        const double low_us = static_cast<double>(i) * static_cast<double>(total.bucket_ns()) / 1000.0;
        rows[row_of(low_us)] += total.bins()[i];
    }
    rows.back() += total.overflow();

    std::size_t first = rows.size();
    std::size_t last = 0;
    std::uint64_t peak = 0;
    for (std::size_t i = 0; i < rows.size(); ++i) {
        if (rows[i] > 0) {
            first = std::min(first, i);
            last = i;
            peak = std::max(peak, rows[i]);
        }
    }
    out << "\nlatency histogram (bar length is log-scaled)\n";
    if (peak == 0) {
        out << "  no samples\n";
        return;
    }
    for (std::size_t i = first; i <= last; ++i) {
        const auto edge = [&kEdgesUs](std::size_t index) {
            return std::to_string(static_cast<int>(kEdgesUs[index]));
        };
        const std::string label = i == 0                 ? "0 - " + edge(0)
                                  : i == kEdgesUs.size() ? ">= " + edge(i - 1)
                                                         : edge(i - 1) + " - " + edge(i);
        const double scale = std::log10(1.0 + static_cast<double>(rows[i])) /
                             std::log10(1.0 + static_cast<double>(peak));
        const auto length = rows[i] == 0 ? 0 : std::max(1, static_cast<int>(std::lround(scale * kBarWidth)));
        out << "  " << std::left << std::setw(14) << label << " us " << std::right << std::setw(10)
            << rows[i] << "  " << std::string(static_cast<std::size_t>(length), '#') << "\n";
    }
}

void print_comparison(std::ostream& out, const Report& a, const Report& b) {
    const Config& c = a.config;
    const ThreadResult& ta = a.threads.front();
    const ThreadResult& tb = b.threads.front();
    out << "rt-latency-bench " << RTLB_VERSION << " compare | mode=" << to_string(a.mode)
        << " | threads=" << c.threads << " | interval=" << c.interval_us
        << " us | load=" << load_text(c) << "\n";
    out << "A = " << describe_policy(ta.policy, ta.priority) << "   B = "
        << describe_policy(tb.policy, tb.priority) << "\n\n";
    out << std::left << std::setw(10) << "metric" << std::right << std::setw(12) << "A (us)"
        << std::setw(12) << "B (us)" << std::setw(12) << "B - A" << std::setw(10) << "B / A" << "\n";

    const Summary sa = summarize(merged_histogram(a));
    const Summary sb = summarize(merged_histogram(b));
    const auto line = [&out](const char* name, double va, double vb) {
        out << std::left << std::setw(10) << name << std::right << std::setw(12) << us1(va)
            << std::setw(12) << us1(vb) << std::setw(12) << us1(vb - va) << std::setw(10)
            << (va > 0.0 ? fixed(vb / va, 2) + "x" : std::string("-")) << "\n";
    };
    line("min", static_cast<double>(sa.min_ns), static_cast<double>(sb.min_ns));
    line("avg", sa.avg_ns, sb.avg_ns);
    line("p50", static_cast<double>(sa.p50_ns), static_cast<double>(sb.p50_ns));
    line("p99", static_cast<double>(sa.p99_ns), static_cast<double>(sb.p99_ns));
    line("p99.9", static_cast<double>(sa.p999_ns), static_cast<double>(sb.p999_ns));
    line("max", static_cast<double>(sa.max_ns), static_cast<double>(sb.max_ns));
    out << std::left << std::setw(10) << "overruns" << std::right << std::setw(12)
        << total_overruns(a) << std::setw(12) << total_overruns(b) << "\n";
    for (const Report* report : {&a, &b}) {
        for (const std::string& warning : report->warnings) out << "warning: " << warning << "\n";
    }
}

void write_csv(std::ostream& out, const Report& report) {
    out << "thread,cpu,policy,samples,min_us,avg_us,max_us,p50_us,p99_us,p999_us,overruns\n";
    for (const Row& row : rows_of(report)) {
        const Summary& s = row.stats;
        out << row.thread << ',' << row.cpu << ',' << row.policy << ',' << s.samples << ','
            << us3(static_cast<double>(s.min_ns)) << ',' << us3(s.avg_ns) << ','
            << us3(static_cast<double>(s.max_ns)) << ',' << us3(static_cast<double>(s.p50_ns)) << ','
            << us3(static_cast<double>(s.p99_ns)) << ',' << us3(static_cast<double>(s.p999_ns)) << ','
            << row.overruns << '\n';
    }
}

void write_json(std::ostream& out, const Report& report) {
    const Config& c = report.config;
    out << "{\n  \"tool\": \"rt-latency-bench\",\n  \"version\": \"" << RTLB_VERSION
        << "\",\n  \"mode\": \"" << to_string(report.mode) << "\",\n  \"config\": {"
        << "\"threads\": " << c.threads << ", \"interval_us\": " << c.interval_us
        << ", \"duration_s\": " << fixed(c.duration_s, 3) << ", \"policy\": \"" << to_string(c.policy)
        << "\", \"priority\": " << c.priority << ", \"mlock\": " << (c.mlock ? "true" : "false")
        << ", \"loads\": [";
    for (std::size_t i = 0; i < c.loads.size(); ++i) {
        out << (i ? ", " : "") << '"' << to_string(c.loads[i]) << '"';
    }
    out << "], \"bucket_us\": " << c.bucket_us << ", \"max_us\": " << c.max_us
        << ", \"seed\": " << c.seed << "},\n  \"warnings\": [";
    for (std::size_t i = 0; i < report.warnings.size(); ++i) {
        out << (i ? ", " : "") << '"' << json_escape(report.warnings[i]) << '"';
    }
    out << "],\n  \"threads\": [\n";
    const std::vector<Row> rows = rows_of(report);
    for (std::size_t i = 0; i + 1 < rows.size(); ++i) {
        out << "    {\"id\": " << rows[i].thread << ", \"cpu\": " << rows[i].cpu << ", \"policy\": \""
            << rows[i].policy << "\", ";
        json_stats(out, rows[i].stats, rows[i].overruns);
        out << (i + 2 < rows.size() ? "},\n" : "}\n");
    }
    out << "  ],\n  \"total\": {";
    json_stats(out, rows.back().stats, rows.back().overruns);
    out << "},\n";

    const Histogram total = merged_histogram(report);
    out << "  \"histogram\": {\"bucket_us\": " << c.bucket_us << ", \"overflow\": " << total.overflow()
        << ", \"bins\": [";
    bool first = true;
    for (std::size_t i = 0; i < total.bins().size(); ++i) {
        if (total.bins()[i] == 0) continue;
        out << (first ? "" : ", ") << '[' << us3(static_cast<double>(i) * static_cast<double>(total.bucket_ns()))
            << ", " << total.bins()[i] << ']';
        first = false;
    }
    out << "]}\n}\n";
}

}  // namespace rtlb
