#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <functional>
#include <unordered_map>

#include "courier/TrackingId.hpp"
#include "courier/ValidationException.hpp"

using courier::TrackingId;
using courier::ValidationException;

TEST_CASE("TrackingId formats a number with prefix and leading zeros") {
    const TrackingId id(42);
    REQUIRE(id.getNumber() == 42);
    REQUIRE(id.getValue() == "CR00000042");
}

TEST_CASE("TrackingId parses valid text") {
    const TrackingId id("CR00000042");
    REQUIRE(id.getNumber() == 42);
    REQUIRE(id.getValue() == "CR00000042");
}

TEST_CASE("TrackingId accepts the full number range") {
    REQUIRE(TrackingId(1).getValue() == "CR00000001");
    REQUIRE(TrackingId(TrackingId::kMaxNumber).getValue() == "CR99999999");
}

TEST_CASE("TrackingId rejects numbers outside the range") {
    REQUIRE_THROWS_AS(TrackingId(0), ValidationException);
    REQUIRE_THROWS_AS(TrackingId(TrackingId::kMaxNumber + 1), ValidationException);
}

TEST_CASE("TrackingId rejects malformed text") {
    REQUIRE_THROWS_AS(TrackingId(""), ValidationException);
    REQUIRE_THROWS_AS(TrackingId("CR0000042"), ValidationException);
    REQUIRE_THROWS_AS(TrackingId("CR000000042"), ValidationException);
    REQUIRE_THROWS_AS(TrackingId("XX00000042"), ValidationException);
    REQUIRE_THROWS_AS(TrackingId("cr00000042"), ValidationException);
    REQUIRE_THROWS_AS(TrackingId("CR0000004A"), ValidationException);
    REQUIRE_THROWS_AS(TrackingId("CR+0000042"), ValidationException);
    REQUIRE_THROWS_AS(TrackingId("CR00000000"), ValidationException);
}

TEST_CASE("TrackingId error message shows the rejected text") {
    REQUIRE_THROWS_WITH(TrackingId("abc"), "Invalid tracking ID: \"abc\"");
}

TEST_CASE("TrackingIds with the same number are equal") {
    REQUIRE(TrackingId(7) == TrackingId("CR00000007"));
    REQUIRE(TrackingId(7) != TrackingId(8));
}

TEST_CASE("Equal TrackingIds have the same hash") {
    const std::hash<TrackingId> hash;
    REQUIRE(hash(TrackingId(7)) == hash(TrackingId("CR00000007")));
}

TEST_CASE("TrackingId works as an unordered_map key") {
    std::unordered_map<TrackingId, int> map;
    map[TrackingId(1)] = 10;
    map[TrackingId(2)] = 20;
    map[TrackingId("CR00000001")] = 11;

    REQUIRE(map.size() == 2);
    REQUIRE(map.at(TrackingId(1)) == 11);
    REQUIRE(map.contains(TrackingId(2)));
    REQUIRE_FALSE(map.contains(TrackingId(3)));
}
