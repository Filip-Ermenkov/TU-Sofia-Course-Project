#include "courier/PackageNotFoundException.hpp"

#include <format>

namespace courier {

PackageNotFoundException::PackageNotFoundException(const std::string& message)
    : LogisticsException(message) {}

PackageNotFoundException::PackageNotFoundException(const TrackingId& id)
    : LogisticsException(std::format("No shipment with tracking ID {}", id.getValue())) {}

}  // namespace courier
