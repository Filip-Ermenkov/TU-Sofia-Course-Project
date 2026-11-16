#include "Validation.hpp"

#include <cmath>

#include "courier/ValidationException.hpp"

namespace courier {

// NaN fails every comparison, so "value <= 0" alone would let it through.
double requirePositive(const std::string& field, double value) {
    if (!std::isfinite(value) || value <= 0.0) {
        throw ValidationException(field, value);
    }
    return value;
}

}  // namespace courier
