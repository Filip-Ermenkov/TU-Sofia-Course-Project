#pragma once

#include <string>

#include "courier/Package.hpp"

namespace courier {

// Regular delivery at the basic rate.
class StandardPackage final : public Package {
public:
    static constexpr Money kBaseFee{500};
    static constexpr Money kPerKg{150};

    StandardPackage(TrackingId id, double weightKg, Dimensions dimensions, std::string recipient,
                    std::string destination, Money declaredValue);
    StandardPackage(TrackingId id, double weightKg, Dimensions dimensions, std::string recipient,
                    std::string destination);

    [[nodiscard]] ServiceType serviceType() const override;
    [[nodiscard]] Money deliveryFee() const override;
};

}  // namespace courier
