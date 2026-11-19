#include "courier/ParcelDetails.hpp"

#include "Validation.hpp"

namespace courier {

ParcelDetails::ParcelDetails(double weightKg, Dimensions dimensions, Money declaredValue)
    : weightKg_(requireWeight(weightKg)),
      dimensions_(dimensions),
      declaredValue_(requireDeclaredValue(declaredValue)) {}

ParcelDetails::ParcelDetails(double weightKg, Dimensions dimensions)
    : ParcelDetails(weightKg, dimensions, Money()) {}

// A Package is always valid, so its values need no second check.
ParcelDetails::ParcelDetails(const Package& package)
    : weightKg_(package.getWeightKg()),
      dimensions_(package.getDimensions()),
      declaredValue_(package.getDeclaredValue()) {}

double ParcelDetails::getWeightKg() const { return weightKg_; }

const Dimensions& ParcelDetails::getDimensions() const { return dimensions_; }

Money ParcelDetails::getDeclaredValue() const { return declaredValue_; }

}  // namespace courier
