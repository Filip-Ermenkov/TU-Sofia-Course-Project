#include <catch2/catch_test_macros.hpp>
#include <stdexcept>

#include "courier/ServiceType.hpp"

using courier::ServiceType;
using courier::toString;

TEST_CASE("ServiceType converts to its name") {
    REQUIRE(toString(ServiceType::Standard) == "Standard");
    REQUIRE(toString(ServiceType::Express) == "Express");
    REQUIRE(toString(ServiceType::Fragile) == "Fragile");
}

TEST_CASE("ServiceType rejects a value outside the enum") {
    REQUIRE_THROWS_AS(toString(static_cast<ServiceType>(99)), std::logic_error);
}
