#include <catch2/catch_test_macros.hpp>
#include <stdexcept>

#include "courier/AssigneeKind.hpp"

using courier::AssigneeKind;
using courier::toString;

TEST_CASE("AssigneeKind converts to its name") {
    REQUIRE(toString(AssigneeKind::Courier) == "Courier");
    REQUIRE(toString(AssigneeKind::Office) == "Office");
}

TEST_CASE("AssigneeKind rejects a value outside the enum") {
    REQUIRE_THROWS_AS(toString(static_cast<AssigneeKind>(99)), std::logic_error);
}
