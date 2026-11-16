#include "Validation.hpp"

#include <cmath>
#include <format>

#include "courier/ValidationException.hpp"

namespace courier {

// NaN fails every comparison, so "value <= 0" alone would let it through.
double requirePositive(const std::string& field, double value) {
    if (!std::isfinite(value) || value <= 0.0) {
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
