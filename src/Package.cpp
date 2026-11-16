#include "courier/Package.hpp"

#include <algorithm>
#include <format>
#include <utility>

#include "Validation.hpp"
#include "courier/LogisticsException.hpp"
#include "courier/ValidationException.hpp"

namespace courier {

namespace {

Money requireNotNegative(Money value) {
    if (value < Money()) {
        throw ValidationException(std::format("Invalid declared value: {}", value.toString()));
    }
    return value;
}

}  // namespace

Package::Package(TrackingId id, double weightKg, Dimensions dimensions, std::string recipient,
                 std::string destination, Money declaredValue)
    : id_(id),
      weightKg_(requirePositive("weight", weightKg)),
      dimensions_(dimensions),
      recipient_(requireNotBlank("recipient", std::move(recipient))),
      destination_(requireNotBlank("destination", std::move(destination))),
      declaredValue_(requireNotNegative(declaredValue)) {}

Package::Package(TrackingId id, double weightKg, Dimensions dimensions, std::string recipient,
                 std::string destination)
    : Package(id, weightKg, dimensions, std::move(recipient), std::move(destination), Money()) {}

const TrackingId& Package::getId() const { return id_; }

double Package::getWeightKg() const { return weightKg_; }

const Dimensions& Package::getDimensions() const { return dimensions_; }

Money Package::getDeclaredValue() const { return declaredValue_; }

const std::string& Package::getRecipient() const { return recipient_; }

const std::string& Package::getDestination() const { return destination_; }

PackageStatus Package::getStatus() const { return status_; }

void Package::setWeightKg(double weightKg) { weightKg_ = requirePositive("weight", weightKg); }

void Package::setDimensions(const Dimensions& dimensions) { dimensions_ = dimensions; }

void Package::setDeclaredValue(Money declaredValue) {
    declaredValue_ = requireNotNegative(declaredValue);
}

void Package::setRecipient(std::string recipient) {
    recipient_ = requireNotBlank("recipient", std::move(recipient));
}

void Package::setDestination(std::string destination) {
    destination_ = requireNotBlank("destination", std::move(destination));
}

void Package::setStatus(PackageStatus status) {
    if (!canTransition(status_, status)) {
        throw LogisticsException(std::format("Cannot change {} from {} to {}", id_.getValue(),
                                             toString(status_), toString(status)));
    }
    status_ = status;
}

double Package::chargeableWeightKg() const {
    return std::max(weightKg_, dimensions_.volume() / kVolumetricDivisor);
}

Money Package::insuranceFee() const { return declaredValue_.multipliedBy(kInsuranceRate); }

Money Package::totalCost() const { return deliveryFee() + insuranceFee(); }

Money Package::weightBasedFee(Money baseFee, Money perKg) const {
    return baseFee + perKg.multipliedBy(chargeableWeightKg());
}

}  // namespace courier
