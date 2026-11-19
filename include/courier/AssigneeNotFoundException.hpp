#pragma once

#include <string>

#include "courier/LogisticsException.hpp"

namespace courier {

// Thrown when no courier or office with the requested code is registered.
class AssigneeNotFoundException : public LogisticsException {
public:
    explicit AssigneeNotFoundException(const std::string& message);
    explicit AssigneeNotFoundException(const char* message);
};

}  // namespace courier
