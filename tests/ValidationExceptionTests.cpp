#include <catch2/catch_test_macros.hpp>
#include <string>

#include "courier/LogisticsException.hpp"
#include "courier/ValidationException.hpp"

TEST_CASE("ValidationException keeps a plain message") {
    const courier::ValidationException error("weight is required");
    REQUIRE(std::string(error.what()) == "weight is required");
}

TEST_CASE("ValidationException builds a message from field and value") {
    const courier::ValidationException error("weight", -3.0);
    REQUIRE(std::string(error.what()) == "Invalid weight: -3");
}

TEST_CASE("ValidationException can be caught as LogisticsException") {
    try {
        throw courier::ValidationException("height", 0.0);
    } catch (const courier::LogisticsException& error) {
        REQUIRE(std::string(error.what()) == "Invalid height: 0");
    }
}
