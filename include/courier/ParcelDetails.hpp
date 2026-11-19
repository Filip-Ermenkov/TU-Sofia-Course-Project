#pragma once

#include "courier/Dimensions.hpp"
#include "courier/Money.hpp"
#include "courier/Package.hpp"

namespace courier {

// The physical parcel and its declared value: everything a modification can change, and
// everything that decides the price. Always valid by the same rules as Package, so a set of new
// details can be checked in full before any of it is applied to a shipment.
// Immutable value type.
class ParcelDetails {
public:
    ParcelDetails(double weightKg, Dimensions dimensions, Money declaredValue);
    // Not insured: declared value 0.00.
    ParcelDetails(double weightKg, Dimensions dimensions);
    // The current details of a shipment.
    explicit ParcelDetails(const Package& package);

    [[nodiscard]] double getWeightKg() const;
    [[nodiscard]] const Dimensions& getDimensions() const;
    [[nodiscard]] Money getDeclaredValue() const;

private:
    double weightKg_;
    Dimensions dimensions_;
    Money declaredValue_;
};

}  // namespace courier
