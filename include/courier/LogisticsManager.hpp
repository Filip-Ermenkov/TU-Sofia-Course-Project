#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

#include "courier/Dimensions.hpp"
#include "courier/Money.hpp"
#include "courier/Package.hpp"
#include "courier/PackageFactory.hpp"
#include "courier/PackageStatus.hpp"
#include "courier/ServiceType.hpp"
#include "courier/StatusObserver.hpp"
#include "courier/TrackingId.hpp"

namespace courier {

// Central register of all shipments. It is their only owner, and it hands out read-only access,
// so every change to a shipment has to go through it. Accepted shipments wait in two delivery
// queues: express ones are dispatched first, the rest in the order they arrived. It is also the
// subject of the Observer pattern: attached observers hear about every status change.
class LogisticsManager {
public:
    // Any callable that answers yes or no for a shipment, typically a lambda.
    using PackagePredicate = std::function<bool(const Package&)>;

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

    // Observers are not owned: each one must outlive the manager or be detached before it is
    // destroyed. Attaching one twice, or detaching one that is not attached, does nothing.
    void attach(StatusObserver& observer);
    void detach(StatusObserver& observer);
    [[nodiscard]] std::size_t observerCount() const;

    // The only way to change a status. Throws PackageNotFoundException for an unknown ID and
    // InvalidStatusChangeException for a step that is not allowed. After a successful change every
    // attached observer is notified; after a failed one, none is.
    void changeStatus(const TrackingId& id, PackageStatus newStatus);
    // Sends the next waiting shipment on its way (In transit) and returns its ID, or nothing if no
    // shipment is waiting.
    std::optional<TrackingId> dispatchNext();
    // Shipments that are accepted but not yet dispatched.
    [[nodiscard]] std::size_t waitingCount() const;

    // Shipments for which the predicate returns true, ordered by tracking ID. The pointers do not
    // own anything and are never null; they stay valid as long as the manager exists.
    [[nodiscard]] std::vector<const Package*> filter(const PackagePredicate& predicate) const;
    // Report of shipments not yet delivered: waiting ones first, then those in transit.
    [[nodiscard]] std::vector<const Package*> undelivered() const;

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
    // Non-owning and never null, notified in the order they were attached.
    std::vector<StatusObserver*> observers_;
};

}  // namespace courier
