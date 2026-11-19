#include <catch2/catch_test_macros.hpp>
#include <string>

#include "courier/AssigneeNotFoundException.hpp"
#include "courier/LogisticsException.hpp"

using courier::AssigneeNotFoundException;

TEST_CASE("AssigneeNotFoundException keeps a std::string message") {
    const std::string message = "No courier or office with code C-01";
    const AssigneeNotFoundException error(message);
    REQUIRE(std::string(error.what()) == message);
}

TEST_CASE("AssigneeNotFoundException keeps a C string message") {
    const AssigneeNotFoundException error("not found");
    REQUIRE(std::string(error.what()) == "not found");
}

TEST_CASE("AssigneeNotFoundException can be caught as LogisticsException") {
    try {
        throw AssigneeNotFoundException("No courier or office with code VAR-1");
    } catch (const courier::LogisticsException& error) {
        REQUIRE(std::string(error.what()) == "No courier or office with code VAR-1");
    }
}
