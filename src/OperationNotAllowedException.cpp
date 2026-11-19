#include "courier/OperationNotAllowedException.hpp"

#include <format>

namespace courier {

OperationNotAllowedException::OperationNotAllowedException(const std::string& message)
    : LogisticsException(message) {}

OperationNotAllowedException::OperationNotAllowedException(const TrackingId& id,
                                                           PackageStatus status)
    : LogisticsException(std::format("Shipment {} is {} and can no longer be changed",
                                     id.getValue(), toString(status))) {}

}  // namespace courier
