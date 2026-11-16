#pragma once

#include <string>

#include "courier/Package.hpp"

namespace courier {

// Priority delivery: higher base fee and rate per kg.
class ExpressPackage final : public Package {
public:
    static constexpr Money kBaseFee{800};
    static constexpr Money kPerKg{250};

    ExpressPackage(TrackingId id, double weightKg, Dimensions dimensions, std::string recipient,
                   std::string destination, Money declaredValue);
    ExpressPackage(TrackingId id, double weightKg, Dimensions dimensions, std::string recipient,
                   std::string destination);

    [[nodiscard]] ServiceType serviceType() const override;
    [[nodiscard]] Money deliveryFee() const override;
};

}  // namespace courier
