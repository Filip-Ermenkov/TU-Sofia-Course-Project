#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "courier/Money.hpp"
#include "courier/TrackingId.hpp"

namespace courier {

// Turning what the user typed into values. These functions only check the format and ignore
// spaces around the text; whether a value is allowed (e.g. a weight up to 1000 kg) is decided by
// the domain classes, which throw ValidationException. Each returns nothing if the text has the
// wrong format.

// The text without spaces, tabs and line breaks at either end. Points into `text`.
[[nodiscard]] std::string_view trim(std::string_view text);

// The text with a-z turned into A-Z; every other character stays as it is.
[[nodiscard]] std::string toUpper(std::string_view text);

// A whole number such as "7" or "-2".
[[nodiscard]] std::optional<int> parseInteger(std::string_view text);

// A finite decimal number such as "2", "2.5" or "-1". Not "inf", "nan" or "2 kg".
[[nodiscard]] std::optional<double> parseNumber(std::string_view text);

// An amount such as "12", "12.5" or "12.50", read exactly into cents. At most two decimals, no
// sign: a price or value the user types is never negative.
[[nodiscard]] std::optional<Money> parseMoney(std::string_view text);

// A tracking ID in any letter case, e.g. "cr00000042".
[[nodiscard]] std::optional<TrackingId> parseTrackingId(std::string_view text);

// A courier or office code as it is registered: trimmed and upper case, so "c-01" finds "C-01".
// Whether it is a valid code is checked where it is used.
[[nodiscard]] std::string normalizeCode(std::string_view text);

}  // namespace courier
