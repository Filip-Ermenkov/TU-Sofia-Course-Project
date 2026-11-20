#pragma once

#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>

#include "courier/OperationRecord.hpp"
#include "courier/Package.hpp"

namespace courier {

// How shipments and history entries are shown on the console.

// The computer's time zone, or nullptr if the system has no time zone database to find it in.
[[nodiscard]] const std::chrono::time_zone* localTimeZone() noexcept;

// E.g. "2026-11-20 21:33:39 +0200" in the given zone, or "2026-11-20 19:33:39 UTC" without one.
// The offset from UTC is shown rather than a name such as "EET", whose spelling differs between
// systems.
[[nodiscard]] std::string formatTime(std::chrono::sys_seconds time,
                                     const std::chrono::time_zone* zone);

// The text cut to at most `width` bytes, ending in "..." if it was longer. A character made of
// several bytes (UTF-8) is never cut in half.
[[nodiscard]] std::string shorten(std::string_view text, std::size_t width);

// Column titles for formatSummary().
[[nodiscard]] std::string summaryHeader();

// One table row: tracking ID, service, status, courier or office, destination and total cost.
[[nodiscard]] std::string formatSummary(const Package& package);

// One history line: when, then what happened, e.g.
// "2026-11-20 21:33:39 +0200  Status change CR00000001: Accepted -> In transit".
[[nodiscard]] std::string formatRecord(const OperationRecord& record,
                                       const std::chrono::time_zone* zone);

}  // namespace courier
