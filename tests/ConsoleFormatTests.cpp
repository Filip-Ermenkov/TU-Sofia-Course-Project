#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <string>

#include "courier/ConsoleFormat.hpp"
#include "courier/Dimensions.hpp"
#include "courier/LogisticsManager.hpp"
#include "courier/Money.hpp"
#include "courier/OperationRecord.hpp"
#include "courier/OperationType.hpp"
#include "courier/ServiceType.hpp"

using courier::OperationRecord;
using courier::OperationType;
using courier::TrackingId;

namespace {

// 2026-11-18 11:06:40 UTC (winter) and 2026-07-02 13:46:40 UTC (summer).
const std::chrono::sys_seconds kWinter{std::chrono::seconds{1'795'000'000}};
const std::chrono::sys_seconds kSummer{std::chrono::seconds{1'783'000'000}};

const std::chrono::time_zone* sofia() { return std::chrono::locate_zone("Europe/Sofia"); }

}  // namespace

TEST_CASE("formatTime shows UTC when there is no time zone") {
    REQUIRE(courier::formatTime(kWinter, nullptr) == "2026-11-18 11:06:40 UTC");
}

TEST_CASE("formatTime shows local time with its offset from UTC") {
    REQUIRE(courier::formatTime(kWinter, sofia()) == "2026-11-18 13:06:40 +0200");
    REQUIRE(courier::formatTime(kSummer, sofia()) == "2026-07-02 16:46:40 +0300");
}

TEST_CASE("localTimeZone never throws") { REQUIRE_NOTHROW(courier::localTimeZone()); }

TEST_CASE("shorten keeps short text and cuts long text with an ellipsis") {
    REQUIRE(courier::shorten("Sofia", 10) == "Sofia");
    REQUIRE(courier::shorten("0123456789", 10) == "0123456789");
    REQUIRE(courier::shorten("0123456789A", 10) == "0123456...");
    REQUIRE(courier::shorten("abcdef", 2) == "..");
    REQUIRE(courier::shorten("", 0).empty());
}

TEST_CASE("shorten never cuts a UTF-8 character in half") {
    // "София" is 10 bytes: each Cyrillic letter takes two.
    const std::string text = "\xD0\xA1\xD0\xBE\xD1\x84\xD0\xB8\xD1\x8F";
    REQUIRE(courier::shorten(text, 8) == "\xD0\xA1\xD0\xBE...");
    REQUIRE(courier::shorten(text, 9) == "\xD0\xA1\xD0\xBE\xD1\x84...");
}

TEST_CASE("formatSummary lines up with the header") {
    courier::LogisticsManager manager;
    const auto& package =
        manager.accept(courier::ServiceType::Express, 2.0, courier::Dimensions(30.0, 20.0, 10.0),
                       "Ivan Petrov", "Varna, a very long street name that does not fit");
    const std::string header = courier::summaryHeader();
    const std::string row = courier::formatSummary(package);
    REQUIRE(header.starts_with("Tracking ID Service   Status      Assigned    Destination"));
    REQUIRE(header.ends_with("Total"));
    REQUIRE(row ==
            "CR00000001  Express   Accepted    -           "
            "Varna, a very long street...       13.00");
    REQUIRE(row.size() == header.size());
}

TEST_CASE("formatRecord names the operation once") {
    const TrackingId id(1);
    REQUIRE(courier::formatRecord(OperationRecord(OperationType::Accept, id,
                                                  "Standard shipment to Sofia for Ivan", kWinter),
                                  nullptr) ==
            "2026-11-18 11:06:40 UTC  Accept CR00000001: Standard shipment to Sofia for Ivan");
    REQUIRE(courier::formatRecord(
                OperationRecord(OperationType::StatusChange, id, "Accepted -> In transit", kWinter),
                nullptr) ==
            "2026-11-18 11:06:40 UTC  Status change CR00000001: Accepted -> In transit");
    REQUIRE(courier::formatRecord(OperationRecord(OperationType::Reroute, id,
                                                  "Reroute CR00000001: Sofia -> Varna", kWinter),
                                  nullptr) ==
            "2026-11-18 11:06:40 UTC  Reroute CR00000001: Sofia -> Varna");
    REQUIRE(
        courier::formatRecord(
            OperationRecord(OperationType::Undo, id, "Reroute CR00000001: Sofia -> Varna", kWinter),
            sofia()) == "2026-11-18 13:06:40 +0200  Undo: Reroute CR00000001: Sofia -> Varna");
}
