#include <catch2/catch_test_macros.hpp>
#include <stdexcept>

#include "courier/ServiceType.hpp"

using courier::parseServiceType;
using courier::ServiceType;
using courier::toString;

TEST_CASE("ServiceType converts to its name") {
    REQUIRE(toString(ServiceType::Standard) == "Standard");
    REQUIRE(toString(ServiceType::Express) == "Express");
    REQUIRE(toString(ServiceType::Fragile) == "Fragile");
}

TEST_CASE("ServiceType is read back from its name") {
    REQUIRE(parseServiceType("Standard") == ServiceType::Standard);
    REQUIRE(parseServiceType("Express") == ServiceType::Express);
    REQUIRE(parseServiceType("Fragile") == ServiceType::Fragile);
}

TEST_CASE("ServiceType parsing rejects any other text") {
    REQUIRE_FALSE(parseServiceType("").has_value());
    REQUIRE_FALSE(parseServiceType("standard").has_value());
    REQUIRE_FALSE(parseServiceType(" Standard").has_value());
}

TEST_CASE("ServiceType rejects a value outside the enum") {
    REQUIRE_THROWS_AS(toString(static_cast<ServiceType>(99)), std::logic_error);
}
