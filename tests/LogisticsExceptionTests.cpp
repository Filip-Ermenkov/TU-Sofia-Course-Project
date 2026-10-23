#include <catch2/catch_test_macros.hpp>
#include <stdexcept>
#include <string>

#include "courier/LogisticsException.hpp"

TEST_CASE("LogisticsException keeps a std::string message") {
    const std::string message = "weight must be positive";
    const courier::LogisticsException error(message);
    REQUIRE(std::string(error.what()) == message);
}

TEST_CASE("LogisticsException keeps a text literal message") {
    const courier::LogisticsException error("weight must be positive");
    REQUIRE(std::string(error.what()) == "weight must be positive");
}

TEST_CASE("LogisticsException can be caught as std::runtime_error") {
    try {
        throw courier::LogisticsException("boom");
    } catch (const std::runtime_error& error) {
        REQUIRE(std::string(error.what()) == "boom");
    }
}
