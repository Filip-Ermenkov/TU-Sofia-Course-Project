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

std::ostream& operator<<(std::ostream& out, const Package& package) {
    const Dimensions& size = package.getDimensions();
    out << std::format("===== WAYBILL {} =====\n", package.getId().getValue())
        << std::format("{:<16}{}\n", "Service:", toString(package.serviceType()))
        << std::format("{:<16}{}\n", "Status:", toString(package.getStatus()))
        << std::format("{:<16}{}\n", "Recipient:", package.getRecipient())
        << std::format("{:<16}{}\n", "Destination:", package.getDestination())
        << std::format("{:<16}{} kg (chargeable {} kg)\n", "Weight:", package.getWeightKg(),
                       package.chargeableWeightKg())
        << std::format("{:<16}{} x {} x {} cm\n", "Size:", size.getLength(), size.getWidth(),
                       size.getHeight())
        << std::format("{:<16}{}\n", "Declared value:", package.getDeclaredValue().toString())
        << std::format("{:<16}{}\n", "Delivery fee:", package.deliveryFee().toString())
        << std::format("{:<16}{}\n", "Insurance:", package.insuranceFee().toString())
        << std::format("{:<16}{}\n", "Total:", package.totalCost().toString());
    return out;
}

}  // namespace courier
