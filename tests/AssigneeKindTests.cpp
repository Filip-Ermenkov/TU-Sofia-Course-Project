#include <catch2/catch_test_macros.hpp>
#include <stdexcept>

#include "courier/AssigneeKind.hpp"

using courier::AssigneeKind;
using courier::parseAssigneeKind;
using courier::toString;

TEST_CASE("AssigneeKind converts to its name") {
    REQUIRE(toString(AssigneeKind::Courier) == "Courier");
    REQUIRE(toString(AssigneeKind::Office) == "Office");
}

TEST_CASE("AssigneeKind is read back from its name") {
    REQUIRE(parseAssigneeKind("Courier") == AssigneeKind::Courier);
    REQUIRE(parseAssigneeKind("Office") == AssigneeKind::Office);
}

TEST_CASE("AssigneeKind parsing rejects any other text") {
    REQUIRE_FALSE(parseAssigneeKind("").has_value());
    REQUIRE_FALSE(parseAssigneeKind("courier").has_value());
    REQUIRE_FALSE(parseAssigneeKind(" Courier").has_value());
}

TEST_CASE("AssigneeKind rejects a value outside the enum") {
    REQUIRE_THROWS_AS(toString(static_cast<AssigneeKind>(99)), std::logic_error);
}
