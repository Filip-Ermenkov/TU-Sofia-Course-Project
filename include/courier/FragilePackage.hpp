#pragma once

#include <string>

#include "courier/Package.hpp"

namespace courier {

// Needs careful handling: standard rate plus a handling fee, and a higher insurance rate.
class FragilePackage final : public Package {
public:
    static constexpr Money kBaseFee{500};
    static constexpr Money kPerKg{150};
    static constexpr Money kHandlingFee{400};
    static constexpr double kFragileInsuranceRate = 0.02;

    FragilePackage(TrackingId id, double weightKg, Dimensions dimensions, std::string recipient,
                   std::string destination, Money declaredValue);
    FragilePackage(TrackingId id, double weightKg, Dimensions dimensions, std::string recipient,
                   std::string destination);

    [[nodiscard]] ServiceType serviceType() const override;
    [[nodiscard]] Money deliveryFee() const override;
    [[nodiscard]] Money insuranceFee() const override;
};

}  // namespace courier
