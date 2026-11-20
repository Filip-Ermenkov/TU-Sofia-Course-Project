#include <catch2/catch_test_macros.hpp>
#include <stdexcept>

#include "courier/PackageStatus.hpp"

using courier::canTransition;
using courier::PackageStatus;
using courier::parsePackageStatus;
using courier::toString;

TEST_CASE("PackageStatus converts to its name") {
    REQUIRE(toString(PackageStatus::Accepted) == "Accepted");
    REQUIRE(toString(PackageStatus::InTransit) == "In transit");
    REQUIRE(toString(PackageStatus::Delivered) == "Delivered");
}

TEST_CASE("PackageStatus is read back from its name") {
    REQUIRE(parsePackageStatus("Accepted") == PackageStatus::Accepted);
    REQUIRE(parsePackageStatus("In transit") == PackageStatus::InTransit);
    REQUIRE(parsePackageStatus("Delivered") == PackageStatus::Delivered);
}

TEST_CASE("PackageStatus parsing rejects any other text") {
    REQUIRE_FALSE(parsePackageStatus("").has_value());
    REQUIRE_FALSE(parsePackageStatus("InTransit").has_value());
    REQUIRE_FALSE(parsePackageStatus(" Accepted").has_value());
}

TEST_CASE("PackageStatus moves forward one step at a time") {
    REQUIRE(canTransition(PackageStatus::Accepted, PackageStatus::InTransit));
    REQUIRE(canTransition(PackageStatus::InTransit, PackageStatus::Delivered));
}

TEST_CASE("PackageStatus rejects skipping, going back and staying") {
    REQUIRE_FALSE(canTransition(PackageStatus::Accepted, PackageStatus::Delivered));
    REQUIRE_FALSE(canTransition(PackageStatus::InTransit, PackageStatus::Accepted));
    REQUIRE_FALSE(canTransition(PackageStatus::Accepted, PackageStatus::Accepted));
}

TEST_CASE("Delivered is a final status") {
    REQUIRE_FALSE(canTransition(PackageStatus::Delivered, PackageStatus::Accepted));
    REQUIRE_FALSE(canTransition(PackageStatus::Delivered, PackageStatus::InTransit));
    REQUIRE_FALSE(canTransition(PackageStatus::Delivered, PackageStatus::Delivered));
}

TEST_CASE("PackageStatus rejects a value outside the enum") {
    const auto invalid = static_cast<PackageStatus>(99);
    REQUIRE_THROWS_AS(toString(invalid), std::logic_error);
    REQUIRE_THROWS_AS(canTransition(invalid, PackageStatus::Accepted), std::logic_error);
}
