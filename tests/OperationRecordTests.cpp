#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <stdexcept>

#include "courier/OperationRecord.hpp"
#include "courier/OperationType.hpp"
#include "courier/ValidationException.hpp"

using courier::OperationRecord;
using courier::OperationType;
using courier::TrackingId;
using courier::ValidationException;

TEST_CASE("OperationType converts to its name") {
    REQUIRE(toString(OperationType::Accept) == "Accept");
    REQUIRE(toString(OperationType::StatusChange) == "Status change");
    REQUIRE(toString(OperationType::Reroute) == "Reroute");
    REQUIRE(toString(OperationType::Modify) == "Modify");
    REQUIRE(toString(OperationType::Assign) == "Assign");
    REQUIRE(toString(OperationType::Undo) == "Undo");
    REQUIRE(toString(OperationType::Redo) == "Redo");
    REQUIRE_THROWS_AS(toString(static_cast<OperationType>(99)), std::logic_error);
}

TEST_CASE("OperationRecord stores the values it was created with") {
    const std::chrono::sys_seconds time{std::chrono::seconds(1'700'000'000)};
    const OperationRecord record(OperationType::Reroute, TrackingId(3), "Reroute CR00000003", time);
    REQUIRE(record.getType() == OperationType::Reroute);
    REQUIRE(record.getId() == TrackingId(3));
    REQUIRE(record.getDetails() == "Reroute CR00000003");
    REQUIRE(record.getTime() == time);
}

TEST_CASE("OperationRecord without a time is stamped with the current time") {
    using std::chrono::floor;
    using std::chrono::seconds;
    using std::chrono::system_clock;
    const auto before = floor<seconds>(system_clock::now());
    const OperationRecord record(OperationType::Accept, TrackingId(1), "Accepted");
    const auto after = floor<seconds>(system_clock::now());
    REQUIRE(record.getTime() >= before);
    REQUIRE(record.getTime() <= after);
}

TEST_CASE("OperationRecord rejects empty details") {
    REQUIRE_THROWS_AS(OperationRecord(OperationType::Undo, TrackingId(1), " "),
                      ValidationException);
}
