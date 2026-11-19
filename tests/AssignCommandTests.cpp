#include <catch2/catch_test_macros.hpp>

#include "courier/AssignCommand.hpp"
#include "courier/Courier.hpp"
#include "courier/Office.hpp"
#include "courier/OperationNotAllowedException.hpp"
#include "courier/StandardPackage.hpp"

using courier::AssignCommand;
using courier::Courier;
using courier::Dimensions;
using courier::Office;
using courier::OperationNotAllowedException;
using courier::PackageStatus;
using courier::StandardPackage;
using courier::TrackingId;

namespace {

StandardPackage makePackage() {
    return StandardPackage(TrackingId(3), 2.0, Dimensions(30.0, 20.0, 10.0), "Georgi Dimitrov",
                           "Varna, 10 Sea Garden");
}

}  // namespace

TEST_CASE("AssignCommand assigns the shipment and undo removes the assignment again") {
    StandardPackage package = makePackage();
    const Courier courier("C-01", "Ivan Petrov");
    AssignCommand command(package, courier);
    REQUIRE(command.getId() == TrackingId(3));
    REQUIRE_FALSE(package.getAssigneeCode().has_value());  // nothing happens before execute

    command.execute();
    REQUIRE(package.getAssigneeCode() == "C-01");
    command.undo();
    REQUIRE_FALSE(package.getAssigneeCode().has_value());
    command.execute();  // redo
    REQUIRE(package.getAssigneeCode() == "C-01");
}

TEST_CASE("AssignCommand reassigns and undo restores the previous assignee") {
    StandardPackage package = makePackage();
    package.setAssigneeCode("C-01");
    AssignCommand command(package, Office("VAR-1", "Varna Center"));
    command.execute();
    REQUIRE(package.getAssigneeCode() == "VAR-1");
    command.undo();
    REQUIRE(package.getAssigneeCode() == "C-01");
}

TEST_CASE("AssignCommand without an assignee removes the assignment") {
    StandardPackage package = makePackage();
    package.setAssigneeCode("C-01");
    AssignCommand command(package);
    command.execute();
    REQUIRE_FALSE(package.getAssigneeCode().has_value());
    command.undo();
    REQUIRE(package.getAssigneeCode() == "C-01");
}

TEST_CASE("AssignCommand describes the change") {
    StandardPackage package = makePackage();
    REQUIRE(AssignCommand(package, Courier("C-01", "Ivan Petrov")).describe() ==
            "Assign CR00000003: none -> C-01");
    package.setAssigneeCode("C-01");
    REQUIRE(AssignCommand(package, Office("VAR-1", "Varna Center")).describe() ==
            "Assign CR00000003: C-01 -> VAR-1");
    REQUIRE(AssignCommand(package).describe() == "Assign CR00000003: C-01 -> none");
}

TEST_CASE("AssignCommand works while the shipment is in transit") {
    StandardPackage package = makePackage();
    package.setStatus(PackageStatus::InTransit);
    AssignCommand command(package, Courier("C-01", "Ivan Petrov"));
    command.execute();
    REQUIRE(package.getAssigneeCode() == "C-01");
}

TEST_CASE("AssignCommand cannot change a delivered shipment") {
    StandardPackage package = makePackage();
    AssignCommand command(package, Courier("C-01", "Ivan Petrov"));
    command.execute();
    package.setStatus(PackageStatus::InTransit);
    package.setStatus(PackageStatus::Delivered);

    REQUIRE_THROWS_AS(command.undo(), OperationNotAllowedException);
    REQUIRE(package.getAssigneeCode() == "C-01");
    AssignCommand unassign(package);
    REQUIRE_THROWS_AS(unassign.execute(), OperationNotAllowedException);
    REQUIRE(package.getAssigneeCode() == "C-01");
}
