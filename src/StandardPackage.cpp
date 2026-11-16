#include "courier/StandardPackage.hpp"

#include <utility>

namespace courier {

StandardPackage::StandardPackage(TrackingId id, double weightKg, Dimensions dimensions,
                                 std::string recipient, std::string destination,
                                 Money declaredValue)
    : Package(id, weightKg, dimensions, std::move(recipient), std::move(destination),
              declaredValue) {}

StandardPackage::StandardPackage(TrackingId id, double weightKg, Dimensions dimensions,
                                 std::string recipient, std::string destination)
    : Package(id, weightKg, dimensions, std::move(recipient), std::move(destination)) {}

ServiceType StandardPackage::serviceType() const { return ServiceType::Standard; }

Money StandardPackage::deliveryFee() const { return weightBasedFee(kBaseFee, kPerKg); }

}  // namespace courier
