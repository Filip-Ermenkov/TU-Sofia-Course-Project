#include <catch2/catch_test_macros.hpp>
#include <sstream>
#include <string>

#include "courier/InputReader.hpp"

using courier::InputReader;
using courier::Money;
using courier::TrackingId;

namespace {

// A reader whose user types `typed`; everything shown ends up in `shown`.
struct Session {
    explicit Session(const std::string& typed) : in(typed), reader(in, shown) {}
    std::istringstream in;
    std::ostringstream shown;
    InputReader reader;
};

}  // namespace

TEST_CASE("InputReader shows the prompt and reads one line") {
    Session session("Sofia\n");
    REQUIRE(session.reader.readLine("City: ") == "Sofia");
    REQUIRE(session.shown.str() == "City: ");
    REQUIRE_FALSE(session.reader.atEnd());
}

TEST_CASE("InputReader reports the end of the input") {
    Session session("");
    REQUIRE_FALSE(session.reader.readLine("City: "));
    REQUIRE(session.reader.atEnd());
    REQUIRE_FALSE(session.reader.askText("Name"));
    REQUIRE_FALSE(session.reader.askNumber("Weight", 2.0));
}

TEST_CASE("InputReader returns trimmed text and cancels on an empty line") {
    Session session("  Ivan Petrov  \n\n");
    REQUIRE(session.reader.askText("Recipient") == "Ivan Petrov");
    REQUIRE_FALSE(session.reader.askText("Recipient"));
    REQUIRE_FALSE(session.reader.atEnd());
    REQUIRE(session.shown.str() == "Recipient: Recipient: ");
}

TEST_CASE("InputReader offers a default that Enter keeps") {
    Session session("\nVarna\n\n");
    REQUIRE(session.reader.askText("Destination", "Sofia") == "Sofia");
    REQUIRE(session.reader.askText("Destination", "Sofia") == "Varna");
    REQUIRE(session.reader.askText("Phone", "") == "");
    REQUIRE(session.shown.str() == "Destination [Sofia]: Destination [Sofia]: Phone [none]: ");
}

TEST_CASE("InputReader asks again until a number is given") {
    Session session("abc\n2 kg\n2.5\n");
    REQUIRE(session.reader.askNumber("Weight (kg)") == 2.5);
    REQUIRE(session.shown.str() ==
            "Weight (kg): "
            "  Please enter a number, e.g. 2.5.\n"
            "Weight (kg): "
            "  Please enter a number, e.g. 2.5.\n"
            "Weight (kg): ");
}

TEST_CASE("InputReader keeps the current number on Enter") {
    Session session("\n");
    REQUIRE(session.reader.askNumber("Weight (kg)", 1.5) == 1.5);
    REQUIRE(session.shown.str() == "Weight (kg) [1.5]: ");
}

TEST_CASE("InputReader reads amounts of money exactly") {
    Session session("12.505\n12.5\n\n");
    REQUIRE(session.reader.askMoney("Declared value") == Money(1250));
    REQUIRE(session.reader.askMoney("Declared value", Money(0)) == Money(0));
    REQUIRE(session.shown.str().find("Declared value [0.00]: ") != std::string::npos);
}

TEST_CASE("InputReader reads tracking IDs and codes in any letter case") {
    Session session("42\ncr00000042\n c-01 \n");
    REQUIRE(session.reader.askTrackingId("Tracking ID") == TrackingId(42));
    REQUIRE(session.reader.askCode("Code") == "C-01");
    REQUIRE(session.shown.str().find("Please enter a tracking ID such as CR00000001.") !=
            std::string::npos);
}

TEST_CASE("InputReader lists the options and returns the chosen position") {
    Session session("0\n4\n2\n");
    REQUIRE(session.reader.askChoice("Service", {"Standard", "Express", "Fragile"}) == 1);
    REQUIRE(session.shown.str() ==
            "  1  Standard\n  2  Express\n  3  Fragile\n"
            "Service: "
            "  Please enter a number from 1 to 3.\n"
            "Service: "
            "  Please enter a number from 1 to 3.\n"
            "Service: ");
}

TEST_CASE("InputReader treats Enter and the end of the input as no") {
    Session session("maybe\nY\nno\n\n");
    REQUIRE(session.reader.askYesNo("Start with no data?"));
    REQUIRE_FALSE(session.reader.askYesNo("Start with no data?"));
    REQUIRE_FALSE(session.reader.askYesNo("Start with no data?"));
    REQUIRE_FALSE(session.reader.askYesNo("Start with no data?"));
    REQUIRE(
        session.shown.str().starts_with("Start with no data? (y/N): "
                                        "  Please answer y or n.\n"));
}
