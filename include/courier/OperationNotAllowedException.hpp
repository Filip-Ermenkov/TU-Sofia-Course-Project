#pragma once

#include <string>

#include "courier/LogisticsException.hpp"
#include "courier/PackageStatus.hpp"
#include "courier/TrackingId.hpp"

namespace courier {

// Thrown when a shipment can no longer be changed, e.g. rerouting a shipment that was delivered.
class OperationNotAllowedException : public LogisticsException {
public:
    explicit OperationNotAllowedException(const std::string& message);
    OperationNotAllowedException(const TrackingId& id, PackageStatus status);
};

}  // namespace courier
