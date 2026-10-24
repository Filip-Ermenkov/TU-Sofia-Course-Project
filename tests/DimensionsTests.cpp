#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <limits>

#include "courier/Dimensions.hpp"
#include "courier/ValidationException.hpp"

using courier::Dimensions;
using courier::ValidationException;

namespace {

constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
constexpr double kInfinity = std::numeric_limits<double>::infinity();

}  // namespace

TEST_CASE("Dimensions stores length, width and height") {
    const Dimensions box(30.0, 20.0, 10.0);
    REQUIRE(box.getLength() == 30.0);
    REQUIRE(box.getWidth() == 20.0);
    REQUIRE(box.getHeight() == 10.0);
}

TEST_CASE("Dimensions with one side is a cube") {
    const Dimensions cube(15.0);
    REQUIRE(cube.getLength() == 15.0);
    REQUIRE(cube.getWidth() == 15.0);
    REQUIRE(cube.getHeight() == 15.0);
}

TEST_CASE("Dimensions computes the volume") {
    const Dimensions box(30.0, 20.0, 10.0);
    REQUIRE(box.volume() == 6000.0);
}

TEST_CASE("Dimensions rejects invalid sides") {
    REQUIRE_THROWS_AS(Dimensions(0.0, 20.0, 10.0), ValidationException);
    REQUIRE_THROWS_AS(Dimensions(30.0, -1.0, 10.0), ValidationException);
    REQUIRE_THROWS_AS(Dimensions(30.0, 20.0, kNaN), ValidationException);
    REQUIRE_THROWS_AS(Dimensions(kInfinity), ValidationException);
}

TEST_CASE("Dimensions error message names the invalid side") {
    REQUIRE_THROWS_WITH(Dimensions(30.0, -2.0, 10.0), "Invalid width: -2");
}

TEST_CASE("Dimensions setter rejects an invalid value and keeps the old one") {
    Dimensions box(30.0, 20.0, 10.0);
    REQUIRE_THROWS_AS(box.setHeight(-5.0), ValidationException);
    REQUIRE(box.getHeight() == 10.0);

    box.setHeight(12.5);
    REQUIRE(box.getHeight() == 12.5);
}
