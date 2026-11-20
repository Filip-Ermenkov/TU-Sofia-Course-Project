#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <string>

#include "courier/InputParsing.hpp"

using courier::Money;
using courier::TrackingId;

TEST_CASE("trim removes spaces, tabs and line breaks at both ends") {
    REQUIRE(courier::trim("  Sofia \t\r\n") == "Sofia");
    REQUIRE(courier::trim("Sofia, 1 Vitosha Blvd") == "Sofia, 1 Vitosha Blvd");
    REQUIRE(courier::trim(" a b ") == "a b");
    REQUIRE(courier::trim("").empty());
    REQUIRE(courier::trim(" \t\r\n").empty());
}

TEST_CASE("toUpper changes only the letters a-z") {
    REQUIRE(courier::toUpper("c-01 Sofia") == "C-01 SOFIA");
    REQUIRE(courier::toUpper("").empty());
}

TEST_CASE("parseInteger reads a whole number and nothing else") {
    REQUIRE(courier::parseInteger("7") == 7);
    REQUIRE(courier::parseInteger(" 19 ") == 19);
    REQUIRE(courier::parseInteger("-2") == -2);
    REQUIRE_FALSE(courier::parseInteger(""));
    REQUIRE_FALSE(courier::parseInteger("abc"));
    REQUIRE_FALSE(courier::parseInteger("1.5"));
    REQUIRE_FALSE(courier::parseInteger("1 2"));
    REQUIRE_FALSE(courier::parseInteger("+3"));
    REQUIRE_FALSE(courier::parseInteger("99999999999999999999"));
}

TEST_CASE("parseNumber reads a finite decimal number") {
    REQUIRE(courier::parseNumber("2") == 2.0);
    REQUIRE(courier::parseNumber(" 2.5 ") == 2.5);
    REQUIRE(courier::parseNumber("-1") == -1.0);  // the domain classes reject it, with a message
    REQUIRE(courier::parseNumber("0") == 0.0);
}

TEST_CASE("parseNumber rejects anything that is not a finite number") {
    REQUIRE_FALSE(courier::parseNumber(""));
    REQUIRE_FALSE(courier::parseNumber("   "));
    REQUIRE_FALSE(courier::parseNumber("abc"));
    REQUIRE_FALSE(courier::parseNumber("2 kg"));
    REQUIRE_FALSE(courier::parseNumber("2,5"));
    REQUIRE_FALSE(courier::parseNumber("inf"));
    REQUIRE_FALSE(courier::parseNumber("nan"));
    REQUIRE_FALSE(courier::parseNumber("1e400"));
}

TEST_CASE("parseMoney reads up to two decimals exactly") {
    REQUIRE(courier::parseMoney("12") == Money(1200));
    REQUIRE(courier::parseMoney("12.5") == Money(1250));
    REQUIRE(courier::parseMoney("12.50") == Money(1250));
    REQUIRE(courier::parseMoney("0.07") == Money(7));
    REQUIRE(courier::parseMoney(" 0 ") == Money(0));
    REQUIRE(courier::parseMoney("1000000.00") == Money(100'000'000));
}

TEST_CASE("parseMoney rejects signs, extra decimals and other formats") {
    for (const char* text : {"", "-1", "+1", "12.505", "12.", ".5", "1e3", "12,50", "12.5.0", "abc",
                             "1 000", "0x10", "99999999999999999999"}) {
        CAPTURE(text);
        REQUIRE_FALSE(courier::parseMoney(text));
    }
}

TEST_CASE("parseMoney rejects amounts too large for whole cents") {
    const std::string largest =
        std::to_string((std::numeric_limits<std::int64_t>::max() - 99) / 100);
    REQUIRE(courier::parseMoney(largest + ".99"));
    REQUIRE_FALSE(courier::parseMoney(largest + "0"));
}

TEST_CASE("parseTrackingId accepts any letter case and surrounding spaces") {
    REQUIRE(courier::parseTrackingId("CR00000042") == TrackingId(42));
    REQUIRE(courier::parseTrackingId(" cr00000042 ") == TrackingId(42));
    REQUIRE_FALSE(courier::parseTrackingId(""));
    REQUIRE_FALSE(courier::parseTrackingId("42"));
    REQUIRE_FALSE(courier::parseTrackingId("CR0000004"));
    REQUIRE_FALSE(courier::parseTrackingId("CR00000000"));
}

TEST_CASE("normalizeCode trims and upper-cases a code") {
    REQUIRE(courier::normalizeCode(" c-01 ") == "C-01");
    REQUIRE(courier::normalizeCode("pld-1") == "PLD-1");
    REQUIRE(courier::normalizeCode("").empty());
}
