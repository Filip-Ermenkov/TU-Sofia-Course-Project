#include "courier/ValidationException.hpp"

#include <format>

namespace courier {

ValidationException::ValidationException(const std::string& message)
    : LogisticsException(message) {}

ValidationException::ValidationException(const std::string& field, double value)
    : LogisticsException(std::format("Invalid {}: {}", field, value)) {}

}  // namespace courier
