#include <catch2/catch_test_macros.hpp>
#include <string>

#include "courier/InvalidStatusChangeException.hpp"
#include "courier/LogisticsException.hpp"

using courier::InvalidStatusChangeException;
using courier::PackageStatus;
using courier::TrackingId;

TEST_CASE("InvalidStatusChangeException keeps a plain message") {
    const InvalidStatusChangeException error("status cannot change");
    REQUIRE(std::string(error.what()) == "status cannot change");
}

TEST_CASE("InvalidStatusChangeException names the shipment and both statuses") {
    const InvalidStatusChangeException error(TrackingId(42), PackageStatus::Delivered,
                                             PackageStatus::InTransit);
    REQUIRE(std::string(error.what()) == "Cannot change CR00000042 from Delivered to In transit");
}

TEST_CASE("InvalidStatusChangeException can be caught as LogisticsException") {
    try {
        throw InvalidStatusChangeException(TrackingId(1), PackageStatus::Accepted,
                                           PackageStatus::Delivered);
    } catch (const courier::LogisticsException& error) {
        REQUIRE(std::string(error.what()) == "Cannot change CR00000001 from Accepted to Delivered");
    }
}
