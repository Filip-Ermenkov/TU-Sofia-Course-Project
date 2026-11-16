#include <catch2/catch_test_macros.hpp>

#include "courier/ExpressPackage.hpp"

using courier::Dimensions;
using courier::ExpressPackage;
using courier::Money;
using courier::ServiceType;
using courier::TrackingId;

TEST_CASE("ExpressPackage reports the express service") {
    const ExpressPackage package(TrackingId(1), 1.0, Dimensions(10.0), "Ivan", "Sofia");
    REQUIRE(package.serviceType() == ServiceType::Express);
}

TEST_CASE("ExpressPackage charges 8.00 plus 2.50 per chargeable kg and 1% insurance") {
    const ExpressPackage package(TrackingId(1), 2.0, Dimensions(30.0, 20.0, 10.0), "Ivan", "Sofia",
                                 Money(10000));
    REQUIRE(package.deliveryFee() == Money(1300));
    REQUIRE(package.insuranceFee() == Money(100));
    REQUIRE(package.totalCost() == Money(1400));
}

TEST_CASE("ExpressPackage charges bulky parcels by volume") {
    const ExpressPackage package(TrackingId(1), 2.0, Dimensions(60.0, 50.0, 40.0), "Ivan", "Sofia");
    REQUIRE(package.deliveryFee() == Money(800 + 250 * 24));
}
