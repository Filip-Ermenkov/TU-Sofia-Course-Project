#pragma once

#include <string>

#include "courier/LogisticsException.hpp"
#include "courier/TrackingId.hpp"

namespace courier {

// Thrown when no shipment with the requested tracking ID exists.
class PackageNotFoundException : public LogisticsException {
public:
    explicit PackageNotFoundException(const std::string& message);
    explicit PackageNotFoundException(const TrackingId& id);
};

}  // namespace courier
