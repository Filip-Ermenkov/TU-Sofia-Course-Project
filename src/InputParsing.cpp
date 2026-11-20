#include "courier/InputParsing.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <limits>
#include <system_error>

#include "courier/ValidationException.hpp"

namespace courier {

namespace {

constexpr std::string_view kBlank = " \t\r\n";

// Plain comparison, not std::isdigit, whose answer depends on the locale.
bool isDigit(char c) { return c >= '0' && c <= '9'; }

// The whole text as a number of type T, or nothing if any character is left over.
template <typename T>
std::optional<T> parseWhole(std::string_view text) {
    T value{};
    const char* const end = text.data() + text.size();
    const auto [stop, error] = std::from_chars(text.data(), end, value);
    if (text.empty() || error != std::errc{} || stop != end) {
        return std::nullopt;
    }
    return value;
}

}  // namespace

std::string_view trim(std::string_view text) {
    const auto first = text.find_first_not_of(kBlank);
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = text.find_last_not_of(kBlank);
    return text.substr(first, last - first + 1);
}

std::string toUpper(std::string_view text) {
    std::string result(text);
    for (char& c : result) {
        if (c >= 'a' && c <= 'z') {
            c = static_cast<char>(c - 'a' + 'A');
        }
    }
    return result;
}

std::optional<int> parseInteger(std::string_view text) { return parseWhole<int>(trim(text)); }

std::optional<double> parseNumber(std::string_view text) {
    const auto value = parseWhole<double>(trim(text));
    // from_chars also reads "inf" and "nan".
    if (!value || !std::isfinite(*value)) {
        return std::nullopt;
    }
    return value;
}

std::optional<Money> parseMoney(std::string_view text) {
    text = trim(text);
    const auto point = text.find('.');
    const std::string_view whole = text.substr(0, point);
    const std::string_view decimals =
        point == std::string_view::npos ? std::string_view{} : text.substr(point + 1);
    const bool wellFormed =
        !whole.empty() && std::ranges::all_of(whole, isDigit) &&
        (point == std::string_view::npos ||
         (!decimals.empty() && decimals.size() <= 2 && std::ranges::all_of(decimals, isDigit)));
    if (!wellFormed) {
        return std::nullopt;
    }

    constexpr std::int64_t kMaxWhole = (std::numeric_limits<std::int64_t>::max() - 99) / 100;
    const auto units = parseWhole<std::int64_t>(whole);
    if (!units || *units > kMaxWhole) {
        return std::nullopt;
    }
    std::int64_t cents = 0;
    if (!decimals.empty()) {
        cents = *parseWhole<std::int64_t>(decimals);
        if (decimals.size() == 1) {
            cents *= 10;  // "12.5" means 12.50
        }
    }
    return Money(*units * 100 + cents);
}

std::optional<TrackingId> parseTrackingId(std::string_view text) {
    try {
        return TrackingId(toUpper(trim(text)));
    } catch (const ValidationException&) {
        return std::nullopt;
    }
}

std::string normalizeCode(std::string_view text) { return toUpper(trim(text)); }

}  // namespace courier
