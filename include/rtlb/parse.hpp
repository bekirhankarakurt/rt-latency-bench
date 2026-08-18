#pragma once

#include <cerrno>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <string>
#include <string_view>
#include <system_error>

namespace rtlb {

/// Parses the whole string as an integer; returns false on any trailing garbage.
template <class T>
[[nodiscard]] bool parse_integer(std::string_view text, T& out) noexcept {
    if (text.empty()) {
        return false;
    }
    T value{};
    const char* const end = text.data() + text.size();
    // NOLINTNEXTLINE(bugprone-suspicious-stringview-data-usage): the range end is passed explicitly.
    const auto result = std::from_chars(text.data(), end, value);
    if (result.ec != std::errc{} || result.ptr != end) {
        return false;
    }
    out = value;
    return true;
}

/// Parses the whole string as a finite decimal number.
[[nodiscard]] inline bool parse_double(std::string_view text, double& out) {
    if (text.empty()) {
        return false;
    }
    const std::string owned(text);
    char* end = nullptr;
    errno = 0;
    const double value = std::strtod(owned.c_str(), &end);
    if (end != owned.c_str() + owned.size() || errno == ERANGE || !std::isfinite(value)) {
        return false;
    }
    out = value;
    return true;
}

}  // namespace rtlb
