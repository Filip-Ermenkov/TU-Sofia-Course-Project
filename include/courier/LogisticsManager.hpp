#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

#include "courier/Assignee.hpp"
#include "courier/CommandHistory.hpp"
#include "courier/Dimensions.hpp"
#include "courier/Money.hpp"
#include "courier/OperationRecord.hpp"
#include "courier/OperationType.hpp"
#include "courier/Package.hpp"
#include "courier/PackageFactory.hpp"
#include "courier/PackageStatus.hpp"
#include "courier/ParcelDetails.hpp"
#include "courier/ServiceType.hpp"
#include "courier/ShipmentCommand.hpp"
#include "courier/StatusObserver.hpp"
#include "courier/TrackingId.hpp"

namespace courier {

// Central register of all shipments. It is their only owner, and it hands out read-only access,
// so every change to a shipment has to go through it. Accepted shipments wait in two delivery
// queues: express ones are dispatched first, the rest in the order they arrived. It is also the
// subject of the Observer pattern: attached observers hear about every status change. Because every
// change passes through it, it also keeps the history of all logistics operations. Rerouting and
// modifying go through the Command pattern, so they can be undone and redone. It also keeps the
// couriers and offices that shipments can be assigned to.
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

    // Undoable changes. Each throws PackageNotFoundException for an unknown ID, ValidationException
    // for an invalid new value and OperationNotAllowedException for a delivered shipment, and then
    // changes nothing. Observers are not notified: none of these changes the status.
    void reroute(const TrackingId& id, std::string destination);
    void reroute(const TrackingId& id, std::string destination, std::string recipient);
    void modify(const TrackingId& id, const ParcelDetails& details);
    // Undo the last change, or redo the last undone one. Return false if there is nothing to do;
    // throw OperationNotAllowedException, changing nothing, if the shipment was delivered since.
    bool undo();
    bool redo();
    [[nodiscard]] bool canUndo() const;
    [[nodiscard]] bool canRedo() const;

    // Registers a courier or office, which the manager then owns. Throws std::invalid_argument for
    // a null pointer and ValidationException if the code is already taken; then registers nothing.
    // Not recorded in the history: it is not an operation on a shipment.
    const Assignee& addAssignee(std::unique_ptr<Assignee> assignee);
    [[nodiscard]] bool hasAssignee(const std::string& code) const;
    // Throws AssigneeNotFoundException for an unknown code.
    [[nodiscard]] const Assignee& getAssignee(const std::string& code) const;
    [[nodiscard]] std::size_t assigneeCount() const;
    // All couriers and offices, ordered by code. Non-owning, never null.
    [[nodiscard]] std::vector<const Assignee*> assignees() const;

    // Every successful operation, oldest first. A failed operation leaves no entry.
    [[nodiscard]] const std::vector<OperationRecord>& history() const;
    // The cancelled operations: every undo ever made, oldest first. A redo or a new change does
    // not remove them.
    [[nodiscard]] std::vector<OperationRecord> cancelledOperations() const;

private:
    [[nodiscard]] Package& findPackage(const TrackingId& id);
    [[nodiscard]] std::queue<TrackingId>& queueFor(const Package& package);
    // Drops IDs of shipments that are no longer waiting and returns the queue whose front is the
    // next shipment to dispatch, or nullptr if none is waiting.
    [[nodiscard]] std::queue<TrackingId>* queueOfNextWaiting();
    // Adds the record, then runs the change. If the change throws, the record is removed again,
    // so the history lists only what really happened.
    void recordAndRun(OperationRecord record, const std::function<void()>& change);
    // Runs a new command through the command history and records it.
    void run(OperationType type, std::unique_ptr<ShipmentCommand> command);

    PackageFactory factory_;
    std::unordered_map<TrackingId, std::unique_ptr<Package>> packages_;
    // IDs, not pointers: the map stays the only owner. An ID whose shipment already left the
    // Accepted status, or was never registered because accept() failed, is skipped when it comes
    // up (std::queue cannot remove from the middle).
    std::queue<TrackingId> expressQueue_;
    std::queue<TrackingId> regularQueue_;
    // Non-owning and never null, notified in the order they were attached.
    std::vector<StatusObserver*> observers_;
    CommandHistory commands_;
    // Keyed by code; Assignee is polymorphic, so each one lives on the heap.
    std::unordered_map<std::string, std::unique_ptr<Assignee>> assignees_;
    std::vector<OperationRecord> history_;
};

}  // namespace courier
