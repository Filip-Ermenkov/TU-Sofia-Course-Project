#pragma once

#include <string>

#include "courier/Dimensions.hpp"
#include "courier/Money.hpp"
#include "courier/PackageStatus.hpp"
#include "courier/ServiceType.hpp"
#include "courier/TrackingId.hpp"

namespace courier {

// A shipment accepted by the courier. Abstract: every service type supplies its own delivery fee.
// The total is always delivery fee + insurance fee (Template Method); subclasses fill in the parts.
class Package {
public:
    // Real weight is charged, unless the parcel is so bulky that its volume weighs more.
    static constexpr double kVolumetricDivisor = 5000.0;  // cm^3 per chargeable kg
    static constexpr double kInsuranceRate = 0.01;        // share of the declared value

    virtual ~Package() = default;

    [[nodiscard]] const TrackingId& getId() const;
    [[nodiscard]] double getWeightKg() const;
    [[nodiscard]] const Dimensions& getDimensions() const;
    [[nodiscard]] Money getDeclaredValue() const;
    [[nodiscard]] const std::string& getRecipient() const;
    [[nodiscard]] const std::string& getDestination() const;
    [[nodiscard]] PackageStatus getStatus() const;

    void setWeightKg(double weightKg);
    void setDimensions(const Dimensions& dimensions);
    void setDeclaredValue(Money declaredValue);
    void setRecipient(std::string recipient);
    void setDestination(std::string destination);
    // Only forward, one step at a time; anything else throws LogisticsException.
    void setStatus(PackageStatus status);

    [[nodiscard]] double chargeableWeightKg() const;

    [[nodiscard]] virtual ServiceType serviceType() const = 0;
    [[nodiscard]] virtual Money deliveryFee() const = 0;
    [[nodiscard]] virtual Money insuranceFee() const;
    [[nodiscard]] Money totalCost() const;

protected:
    Package(TrackingId id, double weightKg, Dimensions dimensions, std::string recipient,
            std::string destination, Money declaredValue);
    Package(TrackingId id, double weightKg, Dimensions dimensions, std::string recipient,
            std::string destination);

    // Protected so a Package cannot be copied or assigned through a base reference (slicing).
    Package(const Package&) = default;
    Package& operator=(const Package&) = default;
    Package(Package&&) = default;
    Package& operator=(Package&&) = default;

    // Shared by the subclasses: base fee plus a rate for every chargeable kilogram.
    [[nodiscard]] Money weightBasedFee(Money baseFee, Money perKg) const;

private:
    TrackingId id_;
    double weightKg_;
    Dimensions dimensions_;
    std::string recipient_;
    std::string destination_;
    Money declaredValue_;
    PackageStatus status_ = PackageStatus::Accepted;
};

}  // namespace courier
