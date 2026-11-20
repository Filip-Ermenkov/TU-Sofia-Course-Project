#include <catch2/catch_test_macros.hpp>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "courier/CsvReader.hpp"
#include "courier/CsvWriter.hpp"
#include "courier/StorageException.hpp"

using courier::CsvReader;
using courier::StorageException;
using Row = std::vector<std::string>;

namespace {

// The message of the StorageException thrown while reading every row of the text.
std::string readError(const std::string& text) {
    std::istringstream in(text);
    CsvReader reader(in, "test.csv");
    try {
        while (reader.readRow()) {
        }
    } catch (const StorageException& error) {
        return error.what();
    }
    return "no error";
}

}  // namespace

TEST_CASE("CsvReader reads one record per line") {
    std::istringstream in("id,status\nCR00000001,Accepted\n");
    CsvReader reader(in);
    REQUIRE(reader.getRowLine() == 0);
    REQUIRE(reader.readRow() == Row{"id", "status"});
    REQUIRE(reader.getRowLine() == 1);
    REQUIRE(reader.readRow() == Row{"CR00000001", "Accepted"});
    REQUIRE(reader.getRowLine() == 2);
    REQUIRE_FALSE(reader.readRow().has_value());
}

TEST_CASE("CsvReader reads quoted fields with commas, quotes and line breaks") {
    std::istringstream in(
        "\"Sofia, 1 Vitosha Blvd\",\"the \"\"Blue\"\" house\",\"line 1\nline 2\"\n"
        "next,row\n");
    CsvReader reader(in);
    REQUIRE(reader.readRow() ==
            Row{"Sofia, 1 Vitosha Blvd", "the \"Blue\" house", "line 1\nline 2"});
    REQUIRE(reader.readRow() == Row{"next", "row"});
    REQUIRE(reader.getRowLine() == 3);  // the quoted line break counts as a line
}

TEST_CASE("CsvReader keeps empty fields") {
    std::istringstream in("C-01,,\n\"\"\n,\n");
    CsvReader reader(in);
    REQUIRE(reader.readRow() == Row{"C-01", "", ""});
    REQUIRE(reader.readRow() == Row{""});
    REQUIRE(reader.readRow() == Row{"", ""});
}

TEST_CASE("CsvReader accepts Windows line endings and a last line without one") {
    std::istringstream in("a,b\r\n\"c\r\nd\",e\r\nf,g");
    CsvReader reader(in);
    REQUIRE(reader.readRow() == Row{"a", "b"});
    REQUIRE(reader.readRow() == Row{"c\r\nd", "e"});
    REQUIRE(reader.readRow() == Row{"f", "g"});
    REQUIRE_FALSE(reader.readRow().has_value());
}

TEST_CASE("CsvReader finds nothing in empty input") {
    std::istringstream in("");
    CsvReader reader(in);
    REQUIRE(reader.getSourceName() == "input");
    REQUIRE_FALSE(reader.readRow().has_value());
}

TEST_CASE("CsvReader reports broken quoting with the source and line") {
    REQUIRE(readError("ok\nab\"c\n") == "test.csv line 2: quote inside an unquoted field");
    REQUIRE(readError("\"abc\"x\n") == "test.csv line 1: unexpected text after a closing quote");
    REQUIRE(readError("ok\n\"abc\nmore\n") == "test.csv line 2: quoted field is not closed");
}

TEST_CASE("CsvReader reports a failed stream") {
    std::istringstream in("a,b\n");
    in.setstate(std::ios::badbit);
    CsvReader reader(in, "history.csv");
    try {
        static_cast<void>(reader.readRow());
        FAIL("expected StorageException");
    } catch (const StorageException& error) {
        REQUIRE(std::string(error.what()) == "Cannot read from history.csv");
    }
}

TEST_CASE("CsvReader reads back exactly what CsvWriter wrote") {
    const std::vector<Row> rows{{"plain", "", "with, comma"},
                                {"\"quoted\"", "two\nlines", "end\r\n"},
                                {""},
                                {"caf\xC3\xA9", " spaces "}};  // UTF-8 bytes pass through unchanged
    std::stringstream buffer;
    courier::CsvWriter writer(buffer);
    for (const Row& row : rows) {
        writer.writeRow(row);
    }
    CsvReader reader(buffer);
    for (const Row& row : rows) {
        REQUIRE(reader.readRow() == row);
    }
    REQUIRE_FALSE(reader.readRow().has_value());
}
