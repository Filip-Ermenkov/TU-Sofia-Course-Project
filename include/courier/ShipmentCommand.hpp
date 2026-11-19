#pragma once

#include <string>

#include "courier/Package.hpp"
#include "courier/TrackingId.hpp"

namespace courier {

// Command pattern: one change to a shipment, packed as an object so it can be undone and redone.
// The shipment it acts on is fixed when the command is created. Each command remembers the values
// from before the change, so execute() and undo() must alternate, starting with execute();
// CommandHistory makes sure of that.
class ShipmentCommand {
public:
    virtual ~ShipmentCommand();

    // A delivered shipment can no longer be changed: both throw OperationNotAllowedException and
    // change nothing in that case.
    void execute();
    void undo();

    [[nodiscard]] const TrackingId& getId() const;
    // One line for the history, e.g. "Reroute CR00000003: Varna -> Burgas".
    [[nodiscard]] virtual std::string describe() const = 0;

    // Bound to one shipment for good, so a command cannot be reassigned.
    ShipmentCommand& operator=(const ShipmentCommand&) = delete;
    ShipmentCommand& operator=(ShipmentCommand&&) = delete;

protected:
    explicit ShipmentCommand(Package& package);
    // Protected, so a command cannot be copied through a base reference (slicing).
    ShipmentCommand(const ShipmentCommand&) = default;
    ShipmentCommand(ShipmentCommand&&) = default;

private:
    // The actual change and its reverse, supplied by each concrete command.
    virtual void doExecute(Package& package) = 0;
    virtual void doUndo(Package& package) = 0;

    void requireChangeable() const;

    Package& package_;
};

}  // namespace courier
