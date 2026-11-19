#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>

#include "courier/Dimensions.hpp"
#include "courier/Money.hpp"
#include "courier/Package.hpp"
#include "courier/PackageFactory.hpp"
#include "courier/ServiceType.hpp"
#include "courier/TrackingId.hpp"

namespace courier {

// Central register of all shipments. It is their only owner, and it hands out read-only access,
// so every change to a shipment has to go through it.
class LogisticsManager {
public:
    // Starts with no shipments; the first one gets CR00000001.
    LogisticsManager();
    // Continues numbering after an ID that was already used, e.g. the highest one in a saved file.
    explicit LogisticsManager(TrackingId lastIssued);

    // Owns the PackageFactory, which cannot be copied or moved, so neither can the manager.
    LogisticsManager(const LogisticsManager&) = delete;
    LogisticsManager& operator=(const LogisticsManager&) = delete;
    LogisticsManager(LogisticsManager&&) = delete;
    LogisticsManager& operator=(LogisticsManager&&) = delete;
    ~LogisticsManager() = default;

    // Creates the shipment with a new tracking ID and registers it. Invalid input throws
    // ValidationException and registers nothing.
    const Package& accept(ServiceType type, double weightKg, Dimensions dimensions,
                          std::string recipient, std::string destination, Money declaredValue);
    const Package& accept(ServiceType type, double weightKg, Dimensions dimensions,
                          std::string recipient, std::string destination);

    [[nodiscard]] bool contains(const TrackingId& id) const;
    // Throws PackageNotFoundException for an unknown ID.
    [[nodiscard]] const Package& get(const TrackingId& id) const;
    [[nodiscard]] std::size_t packageCount() const;

private:
    PackageFactory factory_;
    std::unordered_map<TrackingId, std::unique_ptr<Package>> packages_;
};

}  // namespace courier
