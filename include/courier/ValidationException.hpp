#pragma once

#include <string>

#include "courier/LogisticsException.hpp"

namespace courier {

// Thrown when an input value (weight, dimension, ...) is invalid.
class ValidationException : public LogisticsException {
public:
    explicit ValidationException(const std::string& message);
    ValidationException(const std::string& field, double value);
};

}  // namespace courier
