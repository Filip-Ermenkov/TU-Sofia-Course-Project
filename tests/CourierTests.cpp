#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>

#include "courier/Assignee.hpp"
#include "courier/Courier.hpp"
#include "courier/ValidationException.hpp"

using courier::Assignee;
using courier::AssigneeKind;
using courier::Courier;
using courier::ValidationException;

TEST_CASE("Assignee is an abstract base that cannot be copied through a reference") {
    STATIC_REQUIRE(std::is_abstract_v<Assignee>);
    STATIC_REQUIRE(std::has_virtual_destructor_v<Assignee>);
    STATIC_REQUIRE_FALSE(std::is_copy_assignable_v<Assignee>);
    STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<Assignee>);
}

TEST_CASE("Courier stores its code, name and phone") {
    const Courier courier("C-01", "Ivan Petrov", "0888 123 456");
    REQUIRE(courier.getCode() == "C-01");
    REQUIRE(courier.getName() == "Ivan Petrov");
    REQUIRE(courier.getPhone() == "0888 123 456");
    REQUIRE(courier.kind() == AssigneeKind::Courier);
}

TEST_CASE("Courier can be created without a phone number") {
    const Courier courier("C-02", "Maria Ivanova");
    REQUIRE(courier.getPhone().empty());
}

TEST_CASE("Courier accepts codes of A-Z, 0-9 and dashes up to 10 characters") {
    REQUIRE(Courier("C", "Ivan").getCode() == "C");
    REQUIRE(Courier("SOF-COUR-1", "Ivan").getCode() == "SOF-COUR-1");
}

TEST_CASE("Courier rejects an invalid code") {
    REQUIRE_THROWS_AS(Courier("", "Ivan"), ValidationException);
    REQUIRE_THROWS_AS(Courier("c-01", "Ivan"), ValidationException);  // lower case
    REQUIRE_THROWS_AS(Courier("C 01", "Ivan"), ValidationException);  // space
    REQUIRE_THROWS_AS(Courier("C,01", "Ivan"), ValidationException);  // would break a CSV file
    REQUIRE_THROWS_AS(Courier("SOF-COUR-01", "Ivan"), ValidationException);  // 11 characters
    REQUIRE_THROWS_WITH(Courier("c1", "Ivan"),
                        Catch::Matchers::ContainsSubstring("Invalid code \"c1\""));
}

TEST_CASE("Courier rejects a blank name or phone") {
    REQUIRE_THROWS_AS(Courier("C-01", "  "), ValidationException);
    REQUIRE_THROWS_AS(Courier("C-01", "Ivan", ""), ValidationException);
}

TEST_CASE("Courier setters change valid values and keep the old ones on error") {
    Courier courier("C-01", "Ivan Petrov", "0888 123 456");
    courier.setName("Ivan Georgiev");
    courier.setPhone("0899 000 111");
    REQUIRE(courier.getName() == "Ivan Georgiev");
    REQUIRE(courier.getPhone() == "0899 000 111");

    REQUIRE_THROWS_AS(courier.setName(""), ValidationException);
    REQUIRE_THROWS_AS(courier.setPhone(" "), ValidationException);
    REQUIRE(courier.getName() == "Ivan Georgiev");
    REQUIRE(courier.getPhone() == "0899 000 111");
}

TEST_CASE("Courier describes itself with or without a phone number") {
    REQUIRE(Courier("C-01", "Ivan Petrov", "0888 123 456").describe() ==
            "Courier C-01: Ivan Petrov (phone 0888 123 456)");
    REQUIRE(Courier("C-02", "Maria Ivanova").describe() == "Courier C-02: Maria Ivanova");
}

TEST_CASE("Courier works through a pointer to Assignee") {
    const std::unique_ptr<Assignee> assignee = std::make_unique<Courier>("C-01", "Ivan Petrov");
    REQUIRE(assignee->kind() == AssigneeKind::Courier);
    std::ostringstream out;
    out << *assignee;
    REQUIRE(out.str() == "Courier C-01: Ivan Petrov");
}
