#include "courier/TrackingId.hpp"

#include <charconv>
#include <format>
#include <system_error>

#include "courier/ValidationException.hpp"

namespace courier {

namespace {

constexpr std::string_view kPrefix = "CR";
constexpr std::size_t kDigits = 8;

std::uint32_t requireInRange(std::uint32_t number) {
    if (number == 0 || number > TrackingId::kMaxNumber) {
        throw ValidationException(std::format("Invalid tracking number: {}", number));
    }
    return number;
}

ValidationException invalidText(std::string_view text) {
    return ValidationException(std::format("Invalid tracking ID: \"{}\"", text));
}

std::uint32_t parseNumber(std::string_view text) {
    if (text.size() != kPrefix.size() + kDigits || !text.starts_with(kPrefix)) {
        throw invalidText(text);
    }

    // from_chars reads digits only (no sign, no spaces); it must consume all eight.
    const char* first = text.data() + kPrefix.size();
    const char* last = text.data() + text.size();
    std::uint32_t number = 0;
    const auto result = std::from_chars(first, last, number);
    if (result.ec != std::errc{} || result.ptr != last) {
        throw invalidText(text);
    }
    return number;
}

}  // namespace

TrackingId::TrackingId(std::uint32_t number) : number_(requireInRange(number)) {}

TrackingId::TrackingId(std::string_view text) : TrackingId(parseNumber(text)) {}

std::uint32_t TrackingId::getNumber() const { return number_; }

std::string TrackingId::getValue() const { return std::format("{}{:08}", kPrefix, number_); }

}  // namespace courier
