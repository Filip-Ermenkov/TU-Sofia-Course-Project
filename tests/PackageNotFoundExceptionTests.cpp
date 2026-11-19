#include <catch2/catch_test_macros.hpp>
#include <string>

#include "courier/LogisticsException.hpp"
#include "courier/PackageNotFoundException.hpp"

using courier::PackageNotFoundException;
using courier::TrackingId;

TEST_CASE("PackageNotFoundException keeps a plain message") {
    const PackageNotFoundException error("not found");
    REQUIRE(std::string(error.what()) == "not found");
}

TEST_CASE("PackageNotFoundException names the missing tracking ID") {
    const PackageNotFoundException error(TrackingId(42));
    REQUIRE(std::string(error.what()) == "No shipment with tracking ID CR00000042");
}

TEST_CASE("PackageNotFoundException can be caught as LogisticsException") {
    try {
        throw PackageNotFoundException(TrackingId(7));
    } catch (const courier::LogisticsException& error) {
        REQUIRE(std::string(error.what()) == "No shipment with tracking ID CR00000007");
    }
}
