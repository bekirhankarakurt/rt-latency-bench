#pragma once

#include <string>
#include <vector>

#include "rtlb/config.hpp"

namespace rtlb {

enum class Command { Run, Compare, Help, Version };

struct ParseResult {
    Command command = Command::Run;
    Config config;
    std::string error;  // non-empty when the arguments are invalid
};

// `args` excludes the program name. A leading `run` or `compare` selects the command.
[[nodiscard]] ParseResult parse_args(const std::vector<std::string>& args);
[[nodiscard]] std::string usage();

}  // namespace rtlb
