#include <catch2/catch_test_macros.hpp>
#include <sstream>

#include "courier/Money.hpp"

using courier::Money;

TEST_CASE("Money defaults to zero") { REQUIRE(Money().getCents() == 0); }

TEST_CASE("Money stores whole cents") { REQUIRE(Money(950).getCents() == 950); }

TEST_CASE("Money adds exactly") {
    REQUIRE((Money(10) + Money(20)).getCents() == 30);
    REQUIRE(Money(10) + Money(20) == Money(30));
}

TEST_CASE("Money compares by amount") {
    REQUIRE(Money(100) < Money(101));
    REQUIRE(Money(100) != Money(101));
}

TEST_CASE("Money formats with two decimals") {
    REQUIRE(Money(0).toString() == "0.00");
    REQUIRE(Money(5).toString() == "0.05");
    REQUIRE(Money(900).toString() == "9.00");
    REQUIRE(Money(123456).toString() == "1234.56");
    REQUIRE(Money(-50).toString() == "-0.50");
    REQUIRE(Money(-150).toString() == "-1.50");
}

TEST_CASE("Money multiplication rounds to the nearest cent") {
    REQUIRE(Money(150).multipliedBy(2.0) == Money(300));
    REQUIRE(Money(150).multipliedBy(1.2) == Money(180));
    REQUIRE(Money(10000).multipliedBy(0.01) == Money(100));
    REQUIRE(Money(1).multipliedBy(0.4) == Money(0));
    REQUIRE(Money(1).multipliedBy(0.5) == Money(1));
}

TEST_CASE("Money prints through operator<<") {
    std::ostringstream out;
    out << Money(899);
    REQUIRE(out.str() == "8.99");
}
