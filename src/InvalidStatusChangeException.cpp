#include "courier/InvalidStatusChangeException.hpp"

#include <format>

namespace courier {

InvalidStatusChangeException::InvalidStatusChangeException(const std::string& message)
    : LogisticsException(message) {}

InvalidStatusChangeException::InvalidStatusChangeException(const TrackingId& id, PackageStatus from,
                                                           PackageStatus to)
    : LogisticsException(std::format("Cannot change {} from {} to {}", id.getValue(),
                                     toString(from), toString(to))) {}

}  // namespace courier
