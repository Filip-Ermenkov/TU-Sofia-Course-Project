#include <catch2/catch_test_macros.hpp>
#include <limits>

#include "courier/Package.hpp"
#include "courier/ParcelDetails.hpp"
#include "courier/StandardPackage.hpp"
#include "courier/ValidationException.hpp"

using courier::Dimensions;
using courier::Money;
using courier::Package;
using courier::ParcelDetails;
using courier::StandardPackage;
using courier::TrackingId;
using courier::ValidationException;

TEST_CASE("ParcelDetails stores the values it was created with") {
    const ParcelDetails details(2.5, Dimensions(30.0, 20.0, 10.0), Money(15000));
    REQUIRE(details.getWeightKg() == 2.5);
    REQUIRE(details.getDimensions().getLength() == 30.0);
    REQUIRE(details.getDimensions().getWidth() == 20.0);
    REQUIRE(details.getDimensions().getHeight() == 10.0);
    REQUIRE(details.getDeclaredValue() == Money(15000));
}

TEST_CASE("ParcelDetails without a declared value is not insured") {
    const ParcelDetails details(1.0, Dimensions(10.0));
    REQUIRE(details.getDeclaredValue() == Money());
}

TEST_CASE("ParcelDetails copies the current details of a shipment") {
    const StandardPackage package(TrackingId(1), 4.0, Dimensions(50.0, 40.0, 30.0), "Ivan Petrov",
                                  "Sofia, 1 Vitosha Blvd", Money(2500));
    const ParcelDetails details(package);
    REQUIRE(details.getWeightKg() == 4.0);
    REQUIRE(details.getDimensions().getLength() == 50.0);
    REQUIRE(details.getDimensions().getHeight() == 30.0);
    REQUIRE(details.getDeclaredValue() == Money(2500));
}

TEST_CASE("ParcelDetails rejects an invalid weight") {
    const Dimensions box(10.0);
    REQUIRE_THROWS_AS(ParcelDetails(0.0, box), ValidationException);
    REQUIRE_THROWS_AS(ParcelDetails(-1.0, box), ValidationException);
    REQUIRE_THROWS_AS(ParcelDetails(Package::kMaxWeightKg + 0.1, box), ValidationException);
    REQUIRE_THROWS_AS(ParcelDetails(std::numeric_limits<double>::quiet_NaN(), box),
                      ValidationException);
}

TEST_CASE("ParcelDetails rejects a declared value outside the allowed range") {
    const Dimensions box(10.0);
    REQUIRE_THROWS_AS(ParcelDetails(1.0, box, Money(-1)), ValidationException);
    REQUIRE_THROWS_AS(ParcelDetails(1.0, box, Money(Package::kMaxDeclaredValue.getCents() + 1)),
                      ValidationException);
    REQUIRE_NOTHROW(ParcelDetails(1.0, box, Package::kMaxDeclaredValue));
}
