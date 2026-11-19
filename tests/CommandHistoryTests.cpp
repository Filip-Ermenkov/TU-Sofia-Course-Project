#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <stdexcept>
#include <string>

#include "courier/CommandHistory.hpp"
#include "courier/OperationNotAllowedException.hpp"
#include "courier/RerouteCommand.hpp"
#include "courier/StandardPackage.hpp"
#include "courier/ValidationException.hpp"

using courier::CommandHistory;
using courier::Dimensions;
using courier::OperationNotAllowedException;
using courier::PackageStatus;
using courier::RerouteCommand;
using courier::StandardPackage;
using courier::TrackingId;
using courier::ValidationException;

namespace {

StandardPackage makePackage() {
    return StandardPackage(TrackingId(3), 2.0, Dimensions(30.0, 20.0, 10.0), "Georgi Dimitrov",
                           "Varna");
}

std::unique_ptr<RerouteCommand> reroute(StandardPackage& package, std::string destination) {
    return std::make_unique<RerouteCommand>(package, std::move(destination));
}

}  // namespace

TEST_CASE("CommandHistory starts empty with the default limit") {
    CommandHistory history;
    REQUIRE(history.getLimit() == CommandHistory::kDefaultLimit);
    REQUIRE_FALSE(history.canUndo());
    REQUIRE_FALSE(history.canRedo());
    REQUIRE(history.nextUndo() == nullptr);
    REQUIRE(history.nextRedo() == nullptr);
    REQUIRE_FALSE(history.undo());
    REQUIRE_FALSE(history.redo());
}

TEST_CASE("CommandHistory runs, undoes and redoes commands in order") {
    StandardPackage package = makePackage();
    CommandHistory history;
    history.execute(reroute(package, "Burgas"));
    history.execute(reroute(package, "Ruse"));
    REQUIRE(package.getDestination() == "Ruse");
    REQUIRE(history.undoCount() == 2);
    REQUIRE(history.nextUndo()->describe() == "Reroute CR00000003: Burgas -> Ruse");

    REQUIRE(history.undo());
    REQUIRE(package.getDestination() == "Burgas");
    REQUIRE(history.undo());
    REQUIRE(package.getDestination() == "Varna");
    REQUIRE_FALSE(history.canUndo());
    REQUIRE(history.redoCount() == 2);
    REQUIRE(history.nextRedo()->describe() == "Reroute CR00000003: Varna -> Burgas");

    REQUIRE(history.redo());
    REQUIRE(package.getDestination() == "Burgas");
    REQUIRE(history.redo());
    REQUIRE(package.getDestination() == "Ruse");
    REQUIRE_FALSE(history.canRedo());
}

TEST_CASE("CommandHistory clears the redo stack when a new command runs") {
    StandardPackage package = makePackage();
    CommandHistory history;
    history.execute(reroute(package, "Burgas"));
    REQUIRE(history.undo());
    REQUIRE(history.canRedo());

    history.execute(reroute(package, "Ruse"));
    REQUIRE_FALSE(history.canRedo());
    REQUIRE(history.undoCount() == 1);
}

TEST_CASE("CommandHistory drops the oldest command beyond its limit") {
    StandardPackage package = makePackage();
    CommandHistory history(2);
    history.execute(reroute(package, "Burgas"));
    history.execute(reroute(package, "Ruse"));
    history.execute(reroute(package, "Pleven"));
    REQUIRE(history.undoCount() == 2);
    REQUIRE(history.undo());
    REQUIRE(history.undo());
    REQUIRE_FALSE(history.undo());
    REQUIRE(package.getDestination() == "Burgas");  // the first reroute can no longer be undone
}

TEST_CASE("CommandHistory changes nothing when a command fails") {
    StandardPackage package = makePackage();
    CommandHistory history;
    history.execute(reroute(package, "Burgas"));
    package.setStatus(PackageStatus::InTransit);
    package.setStatus(PackageStatus::Delivered);

    REQUIRE_THROWS_AS(history.execute(reroute(package, "Ruse")), OperationNotAllowedException);
    REQUIRE(history.undoCount() == 1);

    REQUIRE_THROWS_AS(history.undo(), OperationNotAllowedException);
    REQUIRE(history.undoCount() == 1);
    REQUIRE(history.redoCount() == 0);
    REQUIRE(package.getDestination() == "Burgas");
}

TEST_CASE("CommandHistory rejects a bad limit and a null command") {
    REQUIRE_THROWS_AS(CommandHistory(0), ValidationException);
    REQUIRE_THROWS_AS(CommandHistory(CommandHistory::kMaxLimit + 1), ValidationException);
    REQUIRE(CommandHistory(CommandHistory::kMaxLimit).getLimit() == CommandHistory::kMaxLimit);

    CommandHistory history;
    REQUIRE_THROWS_AS(history.execute(nullptr), std::invalid_argument);
}
