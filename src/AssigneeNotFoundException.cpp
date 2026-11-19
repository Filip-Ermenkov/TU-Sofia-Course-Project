#include "courier/AssigneeNotFoundException.hpp"

namespace courier {

AssigneeNotFoundException::AssigneeNotFoundException(const std::string& message)
    : LogisticsException(message) {}

AssigneeNotFoundException::AssigneeNotFoundException(const char* message)
    : LogisticsException(message) {}

}  // namespace courier
