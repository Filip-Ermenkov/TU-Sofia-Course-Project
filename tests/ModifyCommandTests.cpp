#include <catch2/catch_test_macros.hpp>
#include <string>

#include "courier/ModifyCommand.hpp"
#include "courier/OperationNotAllowedException.hpp"
#include "courier/ParcelDetails.hpp"
#include "courier/StandardPackage.hpp"
#include "courier/ValidationException.hpp"

using courier::Dimensions;
using courier::ModifyCommand;
using courier::Money;
using courier::OperationNotAllowedException;
using courier::PackageStatus;
using courier::ParcelDetails;
using courier::StandardPackage;
using courier::TrackingId;
using courier::ValidationException;

namespace {

StandardPackage makePackage() {
    return StandardPackage(TrackingId(3), 2.0, Dimensions(30.0, 20.0, 10.0), "Georgi Dimitrov",
                           "Varna, 10 Sea Garden", Money(10000));
}

}  // namespace

TEST_CASE("ModifyCommand changes the parcel details and undo restores them") {
    StandardPackage package = makePackage();
    const Money oldTotal = package.totalCost();
    ModifyCommand command(package, ParcelDetails(3.5, Dimensions(40.0, 30.0, 20.0), Money(15000)));
    REQUIRE(command.getId() == TrackingId(3));

    command.execute();
    REQUIRE(package.getWeightKg() == 3.5);
    REQUIRE(package.getDimensions().getLength() == 40.0);
    REQUIRE(package.getDeclaredValue() == Money(15000));
    REQUIRE(package.totalCost() > oldTotal);  // the price follows the new details

    command.undo();
    REQUIRE(package.getWeightKg() == 2.0);
    REQUIRE(package.getDimensions().getLength() == 30.0);
    REQUIRE(package.getDimensions().getWidth() == 20.0);
    REQUIRE(package.getDimensions().getHeight() == 10.0);
    REQUIRE(package.getDeclaredValue() == Money(10000));
    REQUIRE(package.totalCost() == oldTotal);
}

TEST_CASE("ModifyCommand can be built from the separate values") {
    StandardPackage package = makePackage();
    ModifyCommand command(package, 5.0, Dimensions(30.0, 20.0, 10.0), Money(10000));
    command.execute();
    REQUIRE(package.getWeightKg() == 5.0);
}

TEST_CASE("ModifyCommand rejects invalid details before anything changes") {
    StandardPackage package = makePackage();
    REQUIRE_THROWS_AS(ModifyCommand(package, -1.0, Dimensions(10.0), Money()), ValidationException);
    REQUIRE_THROWS_AS(ModifyCommand(package, 1.0, Dimensions(10.0), Money(-5)),
                      ValidationException);
    REQUIRE(package.getWeightKg() == 2.0);
    REQUIRE(package.getDeclaredValue() == Money(10000));
}

TEST_CASE("ModifyCommand describes only the values that change") {
    StandardPackage package = makePackage();
    REQUIRE(ModifyCommand(package, 3.5, Dimensions(30.0, 20.0, 10.0), Money(10000)).describe() ==
            "Modify CR00000003: weight 2.00 -> 3.50 kg");
    REQUIRE(ModifyCommand(package, 2.0, Dimensions(40.0, 30.0, 20.0), Money(15000)).describe() ==
            "Modify CR00000003: size 30.0 x 20.0 x 10.0 -> 40.0 x 30.0 x 20.0 cm; declared value "
            "100.00 -> 150.00");
    REQUIRE(ModifyCommand(package, ParcelDetails(package)).describe() ==
            "Modify CR00000003: no changes");
}

TEST_CASE("ModifyCommand cannot change a delivered shipment") {
    StandardPackage package = makePackage();
    ModifyCommand command(package, 3.5, Dimensions(30.0, 20.0, 10.0), Money(10000));
    package.setStatus(PackageStatus::InTransit);
    command.execute();  // still allowed in transit
    package.setStatus(PackageStatus::Delivered);
    REQUIRE_THROWS_AS(command.undo(), OperationNotAllowedException);
    REQUIRE_THROWS_AS(command.execute(), OperationNotAllowedException);
    REQUIRE(package.getWeightKg() == 3.5);
}
