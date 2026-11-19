#include "courier/ExpressPackage.hpp"

#include <utility>

namespace courier {

ExpressPackage::ExpressPackage(TrackingId id, double weightKg, Dimensions dimensions,
                               std::string recipient, std::string destination, Money declaredValue)
    : Package(id, weightKg, dimensions, std::move(recipient), std::move(destination),
              declaredValue) {}

ExpressPackage::ExpressPackage(TrackingId id, double weightKg, Dimensions dimensions,
                               std::string recipient, std::string destination)
    : Package(id, weightKg, dimensions, std::move(recipient), std::move(destination)) {}

ServiceType ExpressPackage::serviceType() const { return ServiceType::Express; }

bool ExpressPackage::isPriority() const { return true; }

Money ExpressPackage::deliveryFee() const { return weightBasedFee(kBaseFee, kPerKg); }

}  // namespace courier
