#include <catch2/catch_test_macros.hpp>
#include <string>

#include "courier/LogisticsException.hpp"
#include "courier/OperationNotAllowedException.hpp"

using courier::OperationNotAllowedException;
using courier::PackageStatus;
using courier::TrackingId;

TEST_CASE("OperationNotAllowedException keeps a plain message") {
    const OperationNotAllowedException error("not allowed");
    REQUIRE(std::string(error.what()) == "not allowed");
}

TEST_CASE("OperationNotAllowedException names the shipment and its status") {
    const OperationNotAllowedException error(TrackingId(42), PackageStatus::Delivered);
    REQUIRE(std::string(error.what()) ==
            "Shipment CR00000042 is Delivered and can no longer be changed");
}

TEST_CASE("OperationNotAllowedException can be caught as LogisticsException") {
    try {
        throw OperationNotAllowedException(TrackingId(7), PackageStatus::Delivered);
    } catch (const courier::LogisticsException& error) {
        REQUIRE(std::string(error.what()) ==
                "Shipment CR00000007 is Delivered and can no longer be changed");
    }
}
