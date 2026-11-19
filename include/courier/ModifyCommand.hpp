#pragma once

#include <string>

#include "courier/Dimensions.hpp"
#include "courier/Money.hpp"
#include "courier/Package.hpp"
#include "courier/ParcelDetails.hpp"
#include "courier/ShipmentCommand.hpp"

namespace courier {

// Changes the weight, size and declared value of a shipment, e.g. after it was measured again at
// the depot. The price follows automatically, because it is always computed from these values.
class ModifyCommand final : public ShipmentCommand {
public:
    ModifyCommand(Package& package, ParcelDetails details);
    ModifyCommand(Package& package, double weightKg, Dimensions dimensions, Money declaredValue);

    // Lists only the values that change, e.g. "Modify CR00000003: weight 2.00 -> 3.50 kg".
    [[nodiscard]] std::string describe() const override;

private:
    void doExecute(Package& package) override;
    void doUndo(Package& package) override;

    ParcelDetails previous_;
    ParcelDetails details_;
};

}  // namespace courier
