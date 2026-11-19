#include "Validation.hpp"

#include <cmath>
#include <format>

#include "courier/Package.hpp"
#include "courier/ValidationException.hpp"

namespace courier {

// NaN fails every comparison, so the range check alone would let it through.
double requireInRange(const std::string& field, double value, double max) {
    if (!std::isfinite(value) || value <= 0.0 || value > max) {
        throw ValidationException(field, value);
    }
    return value;
}

std::string requireNotBlank(const std::string& field, std::string text) {
    if (text.find_first_not_of(" \t\r\n") == std::string::npos) {
        throw ValidationException(std::format("Invalid {}: must not be empty", field));
    }
    return text;
}

double requireWeight(double weightKg) {
    return requireInRange("weight", weightKg, Package::kMaxWeightKg);
}

Money requireDeclaredValue(Money value) {
    if (value < Money() || value > Package::kMaxDeclaredValue) {
        throw ValidationException(std::format("Invalid declared value: {}", value.toString()));
    }
    return value;
}

}  // namespace courier
