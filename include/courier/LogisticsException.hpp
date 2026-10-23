#pragma once

#include <stdexcept>
#include <string>

namespace courier {

// Base class for all errors raised by the courier system.
class LogisticsException : public std::runtime_error {
public:
    explicit LogisticsException(const std::string& message);
    explicit LogisticsException(const char* message);
};

}  // namespace courier
