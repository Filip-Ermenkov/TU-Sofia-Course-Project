#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <sstream>

#include "courier/Assignee.hpp"
#include "courier/Office.hpp"
#include "courier/ValidationException.hpp"

using courier::Assignee;
using courier::AssigneeKind;
using courier::Office;
using courier::ValidationException;

TEST_CASE("Office stores its code, name and address") {
    const Office office("VAR-1", "Varna Center", "10 Sea Garden");
    REQUIRE(office.getCode() == "VAR-1");
    REQUIRE(office.getName() == "Varna Center");
    REQUIRE(office.getAddress() == "10 Sea Garden");
    REQUIRE(office.kind() == AssigneeKind::Office);
}

TEST_CASE("Office can be created without an address") {
    const Office office("SOF-2", "Sofia Lozenets");
    REQUIRE(office.getAddress().empty());
}

TEST_CASE("Office rejects an invalid code, name or address") {
    REQUIRE_THROWS_AS(Office("var-1", "Varna Center"), ValidationException);
    REQUIRE_THROWS_AS(Office("VAR-1", ""), ValidationException);
    REQUIRE_THROWS_AS(Office("VAR-1", "Varna Center", " \t"), ValidationException);
}

TEST_CASE("Office setters change valid values and keep the old ones on error") {
    Office office("VAR-1", "Varna Center", "10 Sea Garden");
    office.setName("Varna Port");
    office.setAddress("2 Port St");
    REQUIRE(office.getName() == "Varna Port");
    REQUIRE(office.getAddress() == "2 Port St");

    REQUIRE_THROWS_AS(office.setAddress(""), ValidationException);
    REQUIRE(office.getAddress() == "2 Port St");
}

TEST_CASE("Office describes itself with or without an address") {
    REQUIRE(Office("VAR-1", "Varna Center", "10 Sea Garden").describe() ==
            "Office VAR-1: Varna Center (10 Sea Garden)");
    REQUIRE(Office("SOF-2", "Sofia Lozenets").describe() == "Office SOF-2: Sofia Lozenets");
}

TEST_CASE("Office works through a pointer to Assignee") {
    const std::unique_ptr<Assignee> assignee =
        std::make_unique<Office>("VAR-1", "Varna Center", "10 Sea Garden");
    REQUIRE(assignee->kind() == AssigneeKind::Office);
    std::ostringstream out;
    out << *assignee;
    REQUIRE(out.str() == "Office VAR-1: Varna Center (10 Sea Garden)");
}
