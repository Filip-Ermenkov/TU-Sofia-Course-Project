#include <catch2/catch_test_macros.hpp>
#include <string>

#include "courier/LogisticsException.hpp"
#include "courier/StorageException.hpp"

using courier::StorageException;

TEST_CASE("StorageException keeps a plain message") {
    const StorageException error("Cannot open data/shipments.csv");
    REQUIRE(std::string(error.what()) == "Cannot open data/shipments.csv");
}

TEST_CASE("StorageException names the file and line") {
    const StorageException error("shipments.csv", 4, "invalid weight");
    REQUIRE(std::string(error.what()) == "shipments.csv line 4: invalid weight");
}

TEST_CASE("StorageException can be caught as LogisticsException") {
    try {
        throw StorageException("history.csv", 2, "unknown operation");
    } catch (const courier::LogisticsException& error) {
        REQUIRE(std::string(error.what()) == "history.csv line 2: unknown operation");
    }
}
