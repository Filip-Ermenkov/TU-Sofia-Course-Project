#include "courier/RerouteCommand.hpp"

#include <format>
#include <utility>

#include "Validation.hpp"

namespace courier {

RerouteCommand::RerouteCommand(Package& package, std::string destination)
    : RerouteCommand(package, std::move(destination), package.getRecipient()) {}

// The new values are checked here, so a bad one is rejected before anything is changed.
RerouteCommand::RerouteCommand(Package& package, std::string destination, std::string recipient)
    : ShipmentCommand(package),
      previousDestination_(package.getDestination()),
      previousRecipient_(package.getRecipient()),
      destination_(requireNotBlank("destination", std::move(destination))),
      recipient_(requireNotBlank("recipient", std::move(recipient))) {}

std::string RerouteCommand::describe() const {
    std::string text =
        std::format("Reroute {}: {} -> {}", getId().getValue(), previousDestination_, destination_);
    if (recipient_ != previousRecipient_) {
        text += std::format("; recipient {} -> {}", previousRecipient_, recipient_);
    }
    return text;
}

// All values are valid, so neither setter can reject them and the change is never half done.
void RerouteCommand::doExecute(Package& package) {
    package.setDestination(destination_);
    package.setRecipient(recipient_);
}

void RerouteCommand::doUndo(Package& package) {
    package.setDestination(previousDestination_);
    package.setRecipient(previousRecipient_);
}

}  // namespace courier
