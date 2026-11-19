#pragma once

#include <optional>
#include <string>

#include "courier/Assignee.hpp"
#include "courier/Package.hpp"
#include "courier/ShipmentCommand.hpp"

namespace courier {

// Assigns a shipment to a courier or an office, or removes its assignment. Undo restores the
// assignment it had before, which may be none.
class AssignCommand final : public ShipmentCommand {
public:
    // Only the assignee's code is kept, so the command does not depend on the Assignee object.
    AssignCommand(Package& package, const Assignee& assignee);
    // Removes the assignment.
    explicit AssignCommand(Package& package);

    [[nodiscard]] std::string describe() const override;

private:
    void doExecute(Package& package) override;
    void doUndo(Package& package) override;

    std::optional<std::string> previousCode_;
    std::optional<std::string> code_;
};

}  // namespace courier
