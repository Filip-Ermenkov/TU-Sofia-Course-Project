#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <limits>
#include <memory>
#include <sstream>
#include <type_traits>

#include "courier/FragilePackage.hpp"
#include "courier/LogisticsException.hpp"
#include "courier/Package.hpp"
#include "courier/StandardPackage.hpp"
#include "courier/ValidationException.hpp"

using courier::Dimensions;
using courier::LogisticsException;
using courier::Money;
using courier::Package;
using courier::PackageStatus;
using courier::StandardPackage;
using courier::TrackingId;
using courier::ValidationException;

// Package is abstract, so its shared behaviour is tested through the simplest subclass.
namespace {

StandardPackage makePackage() {
    return StandardPackage(TrackingId(42), 2.0, Dimensions(30.0, 20.0, 10.0), "Ivan Petrov",
                           "Sofia, 1 Vitosha Blvd", Money(10000));
}

}  // namespace

TEST_CASE("Package is abstract and cannot be copied through a base reference") {
    STATIC_REQUIRE(std::is_abstract_v<Package>);
    STATIC_REQUIRE(std::has_virtual_destructor_v<Package>);
    STATIC_REQUIRE_FALSE(std::is_copy_assignable_v<Package>);
}

TEST_CASE("Package stores the values it was created with") {
    const StandardPackage package = makePackage();
    REQUIRE(package.getId() == TrackingId(42));
    REQUIRE(package.getWeightKg() == 2.0);
    REQUIRE(package.getDimensions().volume() == 6000.0);
    REQUIRE(package.getRecipient() == "Ivan Petrov");
    REQUIRE(package.getDestination() == "Sofia, 1 Vitosha Blvd");
    REQUIRE(package.getDeclaredValue() == Money(10000));
    REQUIRE(package.getStatus() == PackageStatus::Accepted);
}

TEST_CASE("Package without a declared value is not insured") {
    const StandardPackage package(TrackingId(1), 1.0, Dimensions(10.0), "Ivan", "Sofia");
    REQUIRE(package.getDeclaredValue() == Money());
    REQUIRE(package.insuranceFee() == Money());
}

TEST_CASE("Package rejects invalid input on construction") {
    constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
    const Dimensions box(10.0);
    const TrackingId id(1);
    REQUIRE_THROWS_AS(StandardPackage(id, 0.0, box, "Ivan", "Sofia"), ValidationException);
    REQUIRE_THROWS_AS(StandardPackage(id, -1.0, box, "Ivan", "Sofia"), ValidationException);
    REQUIRE_THROWS_AS(StandardPackage(id, kNaN, box, "Ivan", "Sofia"), ValidationException);
    REQUIRE_THROWS_AS(StandardPackage(id, 1.0, box, "", "Sofia"), ValidationException);
    REQUIRE_THROWS_AS(StandardPackage(id, 1.0, box, "Ivan", "   "), ValidationException);
    REQUIRE_THROWS_AS(StandardPackage(id, 1.0, box, "Ivan", "Sofia", Money(-1)),
                      ValidationException);
}

TEST_CASE("Package accepts values up to the limits and rejects larger ones") {
    const Dimensions box(10.0);
    const TrackingId id(1);
    const StandardPackage heaviest(id, Package::kMaxWeightKg, box, "Ivan", "Sofia",
                                   Package::kMaxDeclaredValue);
    REQUIRE(heaviest.getWeightKg() == Package::kMaxWeightKg);

    REQUIRE_THROWS_AS(StandardPackage(id, Package::kMaxWeightKg + 0.5, box, "Ivan", "Sofia"),
                      ValidationException);
    REQUIRE_THROWS_AS(StandardPackage(id, 1e300, box, "Ivan", "Sofia"), ValidationException);
    REQUIRE_THROWS_AS(
        StandardPackage(id, 1.0, box, "Ivan", "Sofia", Package::kMaxDeclaredValue + Money(1)),
        ValidationException);
}

TEST_CASE("The most expensive possible package still has an exact price") {
    // Biggest box and highest value at the most expensive rates: far below the int64 cent range.
    const courier::FragilePackage package(TrackingId(1), Package::kMaxWeightKg,
                                          Dimensions(Dimensions::kMaxSideCm), "Ivan", "Sofia",
                                          Package::kMaxDeclaredValue);
    REQUIRE(package.chargeableWeightKg() == 25000.0);
    REQUIRE(package.totalCost() == Money(500 + 150 * 25000 + 400 + 2'000'000));
}

TEST_CASE("Package setters validate and keep the old value on failure") {
    StandardPackage package = makePackage();

    package.setWeightKg(3.5);
    package.setDimensions(Dimensions(40.0));
    package.setDeclaredValue(Money(500));
    package.setRecipient("Maria Ivanova");
    package.setDestination("Plovdiv");
    REQUIRE(package.getWeightKg() == 3.5);
    REQUIRE(package.getDimensions().getLength() == 40.0);
    REQUIRE(package.getDeclaredValue() == Money(500));
    REQUIRE(package.getRecipient() == "Maria Ivanova");
    REQUIRE(package.getDestination() == "Plovdiv");

    REQUIRE_THROWS_AS(package.setWeightKg(-2.0), ValidationException);
    REQUIRE_THROWS_AS(package.setDeclaredValue(Money(-100)), ValidationException);
    REQUIRE_THROWS_AS(package.setDestination(""), ValidationException);
    REQUIRE(package.getWeightKg() == 3.5);
    REQUIRE(package.getDeclaredValue() == Money(500));
    REQUIRE(package.getDestination() == "Plovdiv");
}

TEST_CASE("Package status moves forward one step at a time") {
    StandardPackage package = makePackage();
    package.setStatus(PackageStatus::InTransit);
    REQUIRE(package.getStatus() == PackageStatus::InTransit);
    package.setStatus(PackageStatus::Delivered);
    REQUIRE(package.getStatus() == PackageStatus::Delivered);
}

TEST_CASE("Package rejects an invalid status change with a clear message") {
    StandardPackage package = makePackage();
    REQUIRE_THROWS_AS(package.setStatus(PackageStatus::Delivered), LogisticsException);
    REQUIRE(package.getStatus() == PackageStatus::Accepted);
    REQUIRE_THROWS_WITH(package.setStatus(PackageStatus::Accepted),
                        "Cannot change CR00000042 from Accepted to Accepted");
}

TEST_CASE("Chargeable weight is the larger of real and volumetric weight") {
    StandardPackage package = makePackage();  // 2 kg, 6000 cm^3 -> 1.2 kg volumetric
    REQUIRE(package.chargeableWeightKg() == 2.0);

    package.setDimensions(Dimensions(60.0, 50.0, 40.0));  // 120000 cm^3 -> 24 kg volumetric
    REQUIRE(package.chargeableWeightKg() == 24.0);
}

TEST_CASE("Package works through a smart pointer to the base class") {
    const std::unique_ptr<Package> package = std::make_unique<StandardPackage>(makePackage());
    REQUIRE(package->serviceType() == courier::ServiceType::Standard);
    REQUIRE(package->totalCost() == Money(900));
}

TEST_CASE("Package prints a waybill with the price breakdown") {
    std::ostringstream out;
    out << makePackage();
    REQUIRE(out.str() ==
            "===== WAYBILL CR00000042 =====\n"
            "Service:        Standard\n"
            "Status:         Accepted\n"
            "Recipient:      Ivan Petrov\n"
            "Destination:    Sofia, 1 Vitosha Blvd\n"
            "Weight:         2 kg (chargeable 2 kg)\n"
            "Size:           30 x 20 x 10 cm\n"
            "Declared value: 100.00\n"
            "Delivery fee:   8.00\n"
            "Insurance:      1.00\n"
            "Total:          9.00\n");
}
