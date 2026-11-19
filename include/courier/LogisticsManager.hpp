#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <unordered_map>

#include "courier/Dimensions.hpp"
#include "courier/Money.hpp"
#include "courier/Package.hpp"
#include "courier/PackageFactory.hpp"
#include "courier/PackageStatus.hpp"
#include "courier/ServiceType.hpp"
#include "courier/TrackingId.hpp"

namespace courier {

// Central register of all shipments. It is their only owner, and it hands out read-only access,
// so every change to a shipment has to go through it. Accepted shipments wait in two delivery
// queues: express ones are dispatched first, the rest in the order they arrived.
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

    // Creates the shipment with a new tracking ID, registers it and queues it for delivery.
    // Invalid input throws ValidationException and registers nothing.
    const Package& accept(ServiceType type, double weightKg, Dimensions dimensions,
                          std::string recipient, std::string destination, Money declaredValue);
    const Package& accept(ServiceType type, double weightKg, Dimensions dimensions,
                          std::string recipient, std::string destination);

    [[nodiscard]] bool contains(const TrackingId& id) const;
    // Throws PackageNotFoundException for an unknown ID.
    [[nodiscard]] const Package& get(const TrackingId& id) const;
    [[nodiscard]] std::size_t packageCount() const;

    // The only way to change a status. Throws PackageNotFoundException for an unknown ID and
    // InvalidStatusChangeException for a step that is not allowed.
    void changeStatus(const TrackingId& id, PackageStatus newStatus);
    // Sends the next waiting shipment on its way (In transit) and returns its ID, or nothing if no
    // shipment is waiting.
    std::optional<TrackingId> dispatchNext();
    // Shipments that are accepted but not yet dispatched.
    [[nodiscard]] std::size_t waitingCount() const;

private:
    [[nodiscard]] Package& findPackage(const TrackingId& id);
    [[nodiscard]] std::queue<TrackingId>& queueFor(const Package& package);
    // Pops IDs until it finds a shipment that is still waiting.
    [[nodiscard]] std::optional<TrackingId> takeNextWaiting();

    PackageFactory factory_;
    std::unordered_map<TrackingId, std::unique_ptr<Package>> packages_;
    // IDs, not pointers: the map stays the only owner. An ID whose shipment already left the
    // Accepted status (std::queue cannot remove from the middle) is skipped when it comes up.
    std::queue<TrackingId> expressQueue_;
    std::queue<TrackingId> regularQueue_;
};

}  // namespace courier
