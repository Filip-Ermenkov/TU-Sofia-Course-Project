#include "Validation.hpp"

#include <cmath>
#include <format>

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

}  // namespace courier
