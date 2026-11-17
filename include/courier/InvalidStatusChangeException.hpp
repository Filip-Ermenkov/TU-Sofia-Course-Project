#pragma once

#include <string>

#include "courier/LogisticsException.hpp"
#include "courier/PackageStatus.hpp"
#include "courier/TrackingId.hpp"

namespace courier {

// Thrown when a shipment is asked to move to a status it cannot reach from its current one.
class InvalidStatusChangeException : public LogisticsException {
public:
    explicit InvalidStatusChangeException(const std::string& message);
    InvalidStatusChangeException(const TrackingId& id, PackageStatus from, PackageStatus to);
};

}  // namespace courier
