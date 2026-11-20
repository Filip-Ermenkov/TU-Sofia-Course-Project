#include "courier/ConsoleFormat.hpp"

#include <format>
#include <stdexcept>

#include "courier/OperationType.hpp"
#include "courier/PackageStatus.hpp"
#include "courier/ServiceType.hpp"

namespace courier {

namespace {

constexpr std::size_t kDestinationWidth = 28;
constexpr std::string_view kEllipsis = "...";

// The columns of summaryHeader() and formatSummary().
constexpr std::string_view kRowFormat = "{:<12}{:<10}{:<12}{:<12}{:<30}{:>10}";

// A byte that continues a UTF-8 character (10xxxxxx) cannot start a piece of text.
bool continuesCharacter(char c) { return (static_cast<unsigned char>(c) & 0xC0U) == 0x80U; }

}  // namespace

const std::chrono::time_zone* localTimeZone() noexcept {
    try {
        return std::chrono::current_zone();
    } catch (const std::exception&) {
        // No time zone database (e.g. an old Windows): times are shown in UTC instead.
        return nullptr;
    }
}

std::string formatTime(std::chrono::sys_seconds time, const std::chrono::time_zone* zone) {
    if (zone == nullptr) {
        return std::format("{:%Y-%m-%d %H:%M:%S} UTC", time);
    }
    return std::format("{:%Y-%m-%d %H:%M:%S %z}", std::chrono::zoned_time(zone, time));
}

std::string shorten(std::string_view text, std::size_t width) {
    if (text.size() <= width) {
        return std::string(text);
    }
    if (width <= kEllipsis.size()) {
        return std::string(kEllipsis.substr(0, width));
    }
    std::size_t length = width - kEllipsis.size();
    while (length > 0 && continuesCharacter(text[length])) {
        --length;
    }
    return std::string(text.substr(0, length)) + std::string(kEllipsis);
}

std::string summaryHeader() {
    return std::format(kRowFormat, "Tracking ID", "Service", "Status", "Assigned", "Destination",
                       "Total");
}

std::string formatSummary(const Package& package) {
    return std::format(kRowFormat, package.getId().getValue(), toString(package.serviceType()),
                       toString(package.getStatus()), package.getAssigneeCode().value_or("-"),
                       shorten(package.getDestination(), kDestinationWidth),
                       package.totalCost().toString());
}

std::string formatRecord(const OperationRecord& record, const std::chrono::time_zone* zone) {
    const std::string time = formatTime(record.getTime(), zone);
    switch (record.getType()) {
        case OperationType::Accept:
        case OperationType::StatusChange:
            return std::format("{}  {} {}: {}", time, toString(record.getType()),
                               record.getId().getValue(), record.getDetails());
        // The details of a command already name the operation and the shipment.
        case OperationType::Reroute:
        case OperationType::Modify:
        case OperationType::Assign:
            return std::format("{}  {}", time, record.getDetails());
        case OperationType::Undo:
        case OperationType::Redo:
            return std::format("{}  {}: {}", time, toString(record.getType()), record.getDetails());
    }
    throw std::logic_error("Unknown OperationType");
}

}  // namespace courier
