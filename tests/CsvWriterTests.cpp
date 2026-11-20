#include <catch2/catch_test_macros.hpp>
#include <sstream>
#include <stdexcept>
#include <string>

#include "courier/CsvWriter.hpp"
#include "courier/StorageException.hpp"

using courier::CsvWriter;
using courier::StorageException;

TEST_CASE("CsvWriter separates fields with commas and ends each record with a newline") {
    std::ostringstream out;
    CsvWriter writer(out);
    writer.writeRow({"CR00000001", "Standard", "2.5"});
    writer.writeRow({"CR00000002", "Express", "10"});
    REQUIRE(out.str() == "CR00000001,Standard,2.5\nCR00000002,Express,10\n");
}

TEST_CASE("CsvWriter quotes fields with commas, quotes and line breaks") {
    std::ostringstream out;
    CsvWriter writer(out);
    writer.writeRow({"Sofia, 1 Vitosha Blvd", "the \"Blue\" house", "line 1\nline 2", "a\rb"});
    REQUIRE(out.str() ==
            "\"Sofia, 1 Vitosha Blvd\",\"the \"\"Blue\"\" house\",\"line 1\nline 2\",\"a\rb\"\n");
}

TEST_CASE("CsvWriter keeps empty fields") {
    std::ostringstream out;
    CsvWriter writer(out);
    writer.writeRow({"C-01", "", ""});
    writer.writeRow({""});
    REQUIRE(out.str() == "C-01,,\n\"\"\n");
}

TEST_CASE("CsvWriter rejects a record with no fields") {
    std::ostringstream out;
    CsvWriter writer(out);
    REQUIRE_THROWS_AS(writer.writeRow({}), std::invalid_argument);
    REQUIRE(out.str().empty());
}

TEST_CASE("CsvWriter reports a failed stream with the target name") {
    std::ostringstream out;
    out.setstate(std::ios::badbit);
    CsvWriter named(out, "shipments.csv");
    REQUIRE(named.getTargetName() == "shipments.csv");
    try {
        named.writeRow({"x"});
        FAIL("expected StorageException");
    } catch (const StorageException& error) {
        REQUIRE(std::string(error.what()) == "Cannot write to shipments.csv");
    }
    CsvWriter unnamed(out);
    REQUIRE(unnamed.getTargetName() == "output");
    REQUIRE_THROWS_AS(unnamed.writeRow({"x"}), StorageException);
}
