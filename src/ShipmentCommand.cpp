#include "courier/ShipmentCommand.hpp"

#include "courier/OperationNotAllowedException.hpp"
#include "courier/PackageStatus.hpp"

namespace courier {

ShipmentCommand::ShipmentCommand(Package& package) : package_(package) {}

ShipmentCommand::~ShipmentCommand() = default;

// Template Method: the rule is checked here, once for every kind of command.
void ShipmentCommand::execute() {
    requireChangeable();
    doExecute(package_);
}

void ShipmentCommand::undo() {
    requireChangeable();
    doUndo(package_);
}

const TrackingId& ShipmentCommand::getId() const { return package_.getId(); }

void ShipmentCommand::requireChangeable() const {
    if (package_.getStatus() == PackageStatus::Delivered) {
        throw OperationNotAllowedException(package_.getId(), package_.getStatus());
    }
}

}  // namespace courier
