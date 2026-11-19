#include <catch2/catch_test_macros.hpp>
#include <string>
#include <type_traits>

#include "courier/OperationNotAllowedException.hpp"
#include "courier/RerouteCommand.hpp"
#include "courier/ShipmentCommand.hpp"
#include "courier/StandardPackage.hpp"
#include "courier/ValidationException.hpp"

using courier::Dimensions;
using courier::OperationNotAllowedException;
using courier::PackageStatus;
using courier::RerouteCommand;
using courier::ShipmentCommand;
using courier::StandardPackage;
using courier::TrackingId;
using courier::ValidationException;

namespace {

StandardPackage makePackage() {
    return StandardPackage(TrackingId(3), 2.0, Dimensions(30.0, 20.0, 10.0), "Georgi Dimitrov",
                           "Varna, 10 Sea Garden");
}

void deliver(StandardPackage& package) {
    package.setStatus(PackageStatus::InTransit);
    package.setStatus(PackageStatus::Delivered);
}

}  // namespace

TEST_CASE("ShipmentCommand is an abstract base that cannot be reassigned") {
    STATIC_REQUIRE(std::is_abstract_v<ShipmentCommand>);
    STATIC_REQUIRE(std::has_virtual_destructor_v<ShipmentCommand>);
    STATIC_REQUIRE_FALSE(std::is_copy_assignable_v<ShipmentCommand>);
    STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<ShipmentCommand>);
}

TEST_CASE("RerouteCommand changes the destination and undo restores it") {
    StandardPackage package = makePackage();
    RerouteCommand command(package, "Burgas, 2 Port St");
    REQUIRE(command.getId() == TrackingId(3));
    REQUIRE(package.getDestination() == "Varna, 10 Sea Garden");  // nothing happens before execute

    command.execute();
    REQUIRE(package.getDestination() == "Burgas, 2 Port St");
    REQUIRE(package.getRecipient() == "Georgi Dimitrov");

    command.undo();
    REQUIRE(package.getDestination() == "Varna, 10 Sea Garden");
    REQUIRE(package.getRecipient() == "Georgi Dimitrov");

    command.execute();  // redo
    REQUIRE(package.getDestination() == "Burgas, 2 Port St");
}

TEST_CASE("RerouteCommand can change the recipient too") {
    StandardPackage package = makePackage();
    RerouteCommand command(package, "Burgas, 2 Port St", "Anna Georgieva");
    command.execute();
    REQUIRE(package.getDestination() == "Burgas, 2 Port St");
    REQUIRE(package.getRecipient() == "Anna Georgieva");
    command.undo();
    REQUIRE(package.getDestination() == "Varna, 10 Sea Garden");
    REQUIRE(package.getRecipient() == "Georgi Dimitrov");
}

TEST_CASE("RerouteCommand describes the change") {
    StandardPackage package = makePackage();
    REQUIRE(RerouteCommand(package, "Burgas").describe() ==
            "Reroute CR00000003: Varna, 10 Sea Garden -> Burgas");
    REQUIRE(RerouteCommand(package, "Burgas", "Anna Georgieva").describe() ==
            "Reroute CR00000003: Varna, 10 Sea Garden -> Burgas; recipient Georgi Dimitrov -> "
            "Anna Georgieva");
}

TEST_CASE("RerouteCommand rejects a blank destination or recipient") {
    StandardPackage package = makePackage();
    REQUIRE_THROWS_AS(RerouteCommand(package, "  "), ValidationException);
    REQUIRE_THROWS_AS(RerouteCommand(package, "Burgas", ""), ValidationException);
    REQUIRE(package.getDestination() == "Varna, 10 Sea Garden");
    REQUIRE(package.getRecipient() == "Georgi Dimitrov");
}

TEST_CASE("RerouteCommand works while the shipment is in transit") {
    StandardPackage package = makePackage();
    package.setStatus(PackageStatus::InTransit);
    RerouteCommand command(package, "Burgas");
    command.execute();
    REQUIRE(package.getDestination() == "Burgas");
}

TEST_CASE("RerouteCommand cannot change a delivered shipment") {
    StandardPackage package = makePackage();
    deliver(package);
    RerouteCommand command(package, "Burgas");
    REQUIRE_THROWS_AS(command.execute(), OperationNotAllowedException);
    REQUIRE(package.getDestination() == "Varna, 10 Sea Garden");
}

TEST_CASE("RerouteCommand cannot be undone once the shipment is delivered") {
    StandardPackage package = makePackage();
    RerouteCommand command(package, "Burgas");
    command.execute();
    deliver(package);
    REQUIRE_THROWS_AS(command.undo(), OperationNotAllowedException);
    REQUIRE(package.getDestination() == "Burgas");
}
