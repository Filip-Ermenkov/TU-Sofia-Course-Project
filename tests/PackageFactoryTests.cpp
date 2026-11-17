#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <type_traits>

#include "courier/ExpressPackage.hpp"
#include "courier/FragilePackage.hpp"
#include "courier/LogisticsException.hpp"
#include "courier/PackageFactory.hpp"
#include "courier/StandardPackage.hpp"
#include "courier/ValidationException.hpp"

using courier::Dimensions;
using courier::ExpressPackage;
using courier::FragilePackage;
using courier::LogisticsException;
using courier::Money;
using courier::Package;
using courier::PackageFactory;
using courier::ServiceType;
using courier::StandardPackage;
using courier::TrackingId;
using courier::ValidationException;

TEST_CASE("PackageFactory creates the subclass that matches the service type") {
    PackageFactory factory;
    const Dimensions box(10.0);

    const auto standard = factory.create(ServiceType::Standard, 1.0, box, "Ivan", "Sofia");
    const auto express = factory.create(ServiceType::Express, 1.0, box, "Ivan", "Sofia");
    const auto fragile = factory.create(ServiceType::Fragile, 1.0, box, "Ivan", "Sofia");

    REQUIRE(dynamic_cast<const StandardPackage*>(standard.get()) != nullptr);
    REQUIRE(dynamic_cast<const ExpressPackage*>(express.get()) != nullptr);
    REQUIRE(dynamic_cast<const FragilePackage*>(fragile.get()) != nullptr);
    REQUIRE(fragile->serviceType() == ServiceType::Fragile);
}

TEST_CASE("PackageFactory passes every value on to the package") {
    PackageFactory factory;
    const auto package = factory.create(ServiceType::Express, 2.0, Dimensions(30.0, 20.0, 10.0),
                                        "Ivan Petrov", "Plovdiv", Money(10000));
    REQUIRE(package->getWeightKg() == 2.0);
    REQUIRE(package->getDimensions().volume() == 6000.0);
    REQUIRE(package->getRecipient() == "Ivan Petrov");
    REQUIRE(package->getDestination() == "Plovdiv");
    REQUIRE(package->getDeclaredValue() == Money(10000));
    REQUIRE(package->totalCost() == Money(800 + 250 * 2 + 100));
}

TEST_CASE("PackageFactory gives every package the next tracking ID") {
    PackageFactory factory;
    REQUIRE_FALSE(factory.getLastIssued().has_value());

    const auto first = factory.create(ServiceType::Standard, 1.0, Dimensions(10.0), "A", "Sofia");
    const auto second = factory.create(ServiceType::Fragile, 1.0, Dimensions(10.0), "B", "Sofia");

    REQUIRE(first->getId() == TrackingId("CR00000001"));
    REQUIRE(second->getId() == TrackingId("CR00000002"));
    REQUIRE(factory.getLastIssued() == TrackingId(2));
}

TEST_CASE("PackageFactory continues after the last issued ID") {
    PackageFactory factory(TrackingId(41));
    const auto package = factory.create(ServiceType::Standard, 1.0, Dimensions(10.0), "A", "Sofia");
    REQUIRE(package->getId() == TrackingId(42));
}

TEST_CASE("PackageFactory does not use up an ID when the input is invalid") {
    PackageFactory factory;
    REQUIRE_THROWS_AS(factory.create(ServiceType::Express, -1.0, Dimensions(10.0), "A", "Sofia"),
                      ValidationException);
    REQUIRE_FALSE(factory.getLastIssued().has_value());

    const auto package = factory.create(ServiceType::Express, 1.0, Dimensions(10.0), "A", "Sofia");
    REQUIRE(package->getId() == TrackingId(1));
}

TEST_CASE("PackageFactory reports when no tracking IDs are left") {
    const TrackingId last(TrackingId::kMaxNumber);
    PackageFactory factory(last);
    REQUIRE_THROWS_AS(factory.create(ServiceType::Standard, 1.0, Dimensions(10.0), "A", "Sofia"),
                      LogisticsException);
    REQUIRE(factory.getLastIssued() == last);
}

TEST_CASE("PackageFactory cannot be copied, so no ID is handed out twice") {
    STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<PackageFactory>);
    STATIC_REQUIRE_FALSE(std::is_copy_assignable_v<PackageFactory>);
}
