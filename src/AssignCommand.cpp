#include "courier/AssignCommand.hpp"

#include <format>

namespace courier {

AssignCommand::AssignCommand(Package& package, const Assignee& assignee)
    : ShipmentCommand(package),
      previousCode_(package.getAssigneeCode()),
      code_(assignee.getCode()) {}

AssignCommand::AssignCommand(Package& package)
    : ShipmentCommand(package), previousCode_(package.getAssigneeCode()) {}

std::string AssignCommand::describe() const {
    return std::format("Assign {}: {} -> {}", getId().getValue(), previousCode_.value_or("none"),
                       code_.value_or("none"));
}

// Both codes are valid (they came from a Package and an Assignee), so the setter cannot reject
// them; it copies the value before it changes anything.
void AssignCommand::doExecute(Package& package) { package.setAssigneeCode(code_); }

void AssignCommand::doUndo(Package& package) { package.setAssigneeCode(previousCode_); }

}  // namespace courier
