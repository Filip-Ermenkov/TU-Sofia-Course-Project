#include "courier/LogisticsException.hpp"

namespace courier {

LogisticsException::LogisticsException(const std::string& message) : std::runtime_error(message) {}

LogisticsException::LogisticsException(const char* message) : std::runtime_error(message) {}

}  // namespace courier
