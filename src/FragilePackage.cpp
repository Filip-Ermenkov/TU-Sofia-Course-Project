#include "courier/FragilePackage.hpp"

#include <utility>

namespace courier {

FragilePackage::FragilePackage(TrackingId id, double weightKg, Dimensions dimensions,
                               std::string recipient, std::string destination, Money declaredValue)
    : Package(id, weightKg, dimensions, std::move(recipient), std::move(destination),
              declaredValue) {}

FragilePackage::FragilePackage(TrackingId id, double weightKg, Dimensions dimensions,
                               std::string recipient, std::string destination)
    : Package(id, weightKg, dimensions, std::move(recipient), std::move(destination)) {}

ServiceType FragilePackage::serviceType() const { return ServiceType::Fragile; }

bool FragilePackage::isPriority() const { return true; }

Money FragilePackage::deliveryFee() const {
    return weightBasedFee(kBaseFee, kPerKg) + kHandlingFee;
}

Money FragilePackage::insuranceFee() const {
    return getDeclaredValue().multipliedBy(kFragileInsuranceRate);
}

}  // namespace courier
