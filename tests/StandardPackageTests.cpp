#include <catch2/catch_test_macros.hpp>

#include "courier/StandardPackage.hpp"

using courier::Dimensions;
using courier::Money;
using courier::ServiceType;
using courier::StandardPackage;
using courier::TrackingId;

TEST_CASE("StandardPackage reports the standard service") {
    const StandardPackage package(TrackingId(1), 1.0, Dimensions(10.0), "Ivan", "Sofia");
    REQUIRE(package.serviceType() == ServiceType::Standard);
}

TEST_CASE("StandardPackage is not a priority shipment") {
    const StandardPackage package(TrackingId(1), 1.0, Dimensions(10.0), "Ivan", "Sofia");
    REQUIRE_FALSE(package.isPriority());
}

TEST_CASE("StandardPackage charges 5.00 plus 1.50 per chargeable kg and 1% insurance") {
    const StandardPackage package(TrackingId(1), 2.0, Dimensions(30.0, 20.0, 10.0), "Ivan", "Sofia",
                                  Money(10000));
    REQUIRE(package.deliveryFee() == Money(800));
    REQUIRE(package.insuranceFee() == Money(100));
    REQUIRE(package.totalCost() == Money(900));
}

TEST_CASE("StandardPackage charges bulky parcels by volume") {
    const StandardPackage package(TrackingId(1), 2.0, Dimensions(60.0, 50.0, 40.0), "Ivan",
                                  "Sofia");
    REQUIRE(package.deliveryFee() == Money(500 + 150 * 24));
    REQUIRE(package.totalCost() == Money(4100));
}
