#pragma once

#include <string>

#include "courier/Package.hpp"
#include "courier/ShipmentCommand.hpp"

namespace courier {

// Sends a shipment to a new destination, optionally to a new recipient as well.
class RerouteCommand final : public ShipmentCommand {
public:
    // Same recipient, new destination.
    RerouteCommand(Package& package, std::string destination);
    RerouteCommand(Package& package, std::string destination, std::string recipient);

    [[nodiscard]] std::string describe() const override;

private:
    void doExecute(Package& package) override;
    void doUndo(Package& package) override;

    std::string previousDestination_;
    std::string previousRecipient_;
    std::string destination_;
    std::string recipient_;
};

}  // namespace courier
