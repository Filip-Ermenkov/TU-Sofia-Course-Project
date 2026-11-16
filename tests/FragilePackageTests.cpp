#include <catch2/catch_test_macros.hpp>
#include <memory>

#include "courier/FragilePackage.hpp"

using courier::Dimensions;
using courier::FragilePackage;
using courier::Money;
using courier::Package;
using courier::ServiceType;
using courier::TrackingId;

TEST_CASE("FragilePackage reports the fragile service") {
    const FragilePackage package(TrackingId(1), 1.0, Dimensions(10.0), "Ivan", "Sofia");
    REQUIRE(package.serviceType() == ServiceType::Fragile);
}

TEST_CASE("FragilePackage adds a 4.00 handling fee and charges 2% insurance") {
    const FragilePackage package(TrackingId(1), 2.0, Dimensions(30.0, 20.0, 10.0), "Ivan", "Sofia",
                                 Money(10000));
    REQUIRE(package.deliveryFee() == Money(1200));
    REQUIRE(package.insuranceFee() == Money(200));
    REQUIRE(package.totalCost() == Money(1400));
}

TEST_CASE("FragilePackage insurance override is used through the base class") {
    const std::unique_ptr<Package> package = std::make_unique<FragilePackage>(
        TrackingId(1), 2.0, Dimensions(30.0, 20.0, 10.0), "Ivan", "Sofia", Money(10000));
    REQUIRE(package->insuranceFee() == Money(200));
}
