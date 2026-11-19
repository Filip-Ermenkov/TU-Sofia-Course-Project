#include "courier/LogisticsManager.hpp"

#include <algorithm>
#include <format>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <ranges>
#include <utility>

#include "courier/ModifyCommand.hpp"
#include "courier/OperationType.hpp"
#include "courier/PackageNotFoundException.hpp"
#include "courier/RerouteCommand.hpp"

namespace courier {

LogisticsManager::LogisticsManager() = default;

LogisticsManager::LogisticsManager(TrackingId lastIssued) : factory_(lastIssued) {}

const Package& LogisticsManager::accept(ServiceType type, double weightKg, Dimensions dimensions,
                                        std::string recipient, std::string destination,
                                        Money declaredValue) {
    auto package = factory_.create(type, weightKg, dimensions, std::move(recipient),
                                   std::move(destination), declaredValue);
    // The factory never repeats an ID, so the key is always new.
    const TrackingId id = package->getId();
    const auto it = packages_.emplace(id, std::move(package)).first;
    const Package& accepted = *it->second;
    try {
        queueFor(accepted).push(id);
        history_.emplace_back(OperationType::Accept, id,
                              std::format("{} shipment to {} for {}", toString(type),
                                          accepted.getDestination(), accepted.getRecipient()));
    } catch (...) {
        // Out of memory: undo the registration, so nothing half-done stays. An ID already in the
        // queue is skipped later, because its shipment is not registered.
        packages_.erase(it);
        throw;
    }
    return accepted;
}

const Package& LogisticsManager::accept(ServiceType type, double weightKg, Dimensions dimensions,
                                        std::string recipient, std::string destination) {
    return accept(type, weightKg, dimensions, std::move(recipient), std::move(destination),
                  Money());
}

bool LogisticsManager::contains(const TrackingId& id) const { return packages_.contains(id); }

const Package& LogisticsManager::get(const TrackingId& id) const {
    const auto it = packages_.find(id);
    if (it == packages_.end()) {
        throw PackageNotFoundException(id);
    }
    return *it->second;
}

std::size_t LogisticsManager::packageCount() const { return packages_.size(); }

void LogisticsManager::attach(StatusObserver& observer) {
    if (std::ranges::find(observers_, &observer) == observers_.end()) {
        observers_.push_back(&observer);
    }
}

void LogisticsManager::detach(StatusObserver& observer) { std::erase(observers_, &observer); }

std::size_t LogisticsManager::observerCount() const { return observers_.size(); }

void LogisticsManager::changeStatus(const TrackingId& id, PackageStatus newStatus) {
    Package& package = findPackage(id);
    const PackageStatus previous = package.getStatus();
    // setStatus throws, and nothing is recorded or told, if the step is not allowed.
    recordAndRun(OperationRecord(OperationType::StatusChange, id,
                                 std::format("{} -> {}", toString(previous), toString(newStatus))),
                 [&package, newStatus] { package.setStatus(newStatus); });
    for (StatusObserver* observer : observers_) {
        observer->onStatusChanged(package, previous);
    }
}

std::optional<TrackingId> LogisticsManager::dispatchNext() {
    std::queue<TrackingId>* queue = queueOfNextWaiting();
    if (queue == nullptr) {
        return std::nullopt;
    }
    const TrackingId id = queue->front();
    changeStatus(id, PackageStatus::InTransit);
    queue->pop();  // only now: if the change throws, the shipment keeps its place in the queue
    return id;
}

std::size_t LogisticsManager::waitingCount() const {
    const auto waiting = std::ranges::count_if(packages_, [](const auto& entry) {
        return entry.second->getStatus() == PackageStatus::Accepted;
    });
    return static_cast<std::size_t>(waiting);
}

std::vector<const Package*> LogisticsManager::filter(const PackagePredicate& predicate) const {
    std::vector<const Package*> result;
    for (const auto& package : packages_ | std::views::values) {
        if (predicate(*package)) {
            result.push_back(package.get());
        }
    }
    // The map keeps no order, so sort to give the same answer every time.
    std::ranges::sort(result, {},
                      [](const Package* package) { return package->getId().getNumber(); });
    return result;
}

std::vector<const Package*> LogisticsManager::undelivered() const {
    auto result = filter(
        [](const Package& package) { return package.getStatus() != PackageStatus::Delivered; });
    // Stable, so the tracking ID order from filter() is kept within each status.
    std::ranges::stable_sort(result, {},
                             [](const Package* package) { return package->getStatus(); });
    return result;
}

void LogisticsManager::reroute(const TrackingId& id, std::string destination) {
    run(OperationType::Reroute,
        std::make_unique<RerouteCommand>(findPackage(id), std::move(destination)));
}

void LogisticsManager::reroute(const TrackingId& id, std::string destination,
                               std::string recipient) {
    run(OperationType::Reroute, std::make_unique<RerouteCommand>(
                                    findPackage(id), std::move(destination), std::move(recipient)));
}

void LogisticsManager::modify(const TrackingId& id, const ParcelDetails& details) {
    run(OperationType::Modify, std::make_unique<ModifyCommand>(findPackage(id), details));
}

// The record is built from the command before it is undone, while its text still describes the
// change being cancelled.
bool LogisticsManager::undo() {
    const ShipmentCommand* next = commands_.nextUndo();
    if (next == nullptr) {
        return false;
    }
    recordAndRun(OperationRecord(OperationType::Undo, next->getId(), next->describe()),
                 [this] { commands_.undo(); });
    return true;
}

bool LogisticsManager::redo() {
    const ShipmentCommand* next = commands_.nextRedo();
    if (next == nullptr) {
        return false;
    }
    recordAndRun(OperationRecord(OperationType::Redo, next->getId(), next->describe()),
                 [this] { commands_.redo(); });
    return true;
}

bool LogisticsManager::canUndo() const { return commands_.canUndo(); }

bool LogisticsManager::canRedo() const { return commands_.canRedo(); }

const std::vector<OperationRecord>& LogisticsManager::history() const { return history_; }

std::vector<OperationRecord> LogisticsManager::cancelledOperations() const {
    std::vector<OperationRecord> cancelled;
    std::ranges::copy_if(
        history_, std::back_inserter(cancelled),
        [](const OperationRecord& record) { return record.getType() == OperationType::Undo; });
    return cancelled;
}

// Same lookup as get(), but gives write access. Private and non-const, so only the manager's own
// changing functions can modify a shipment.
Package& LogisticsManager::findPackage(const TrackingId& id) {
    const auto it = packages_.find(id);
    if (it == packages_.end()) {
        throw PackageNotFoundException(id);
    }
    return *it->second;
}

std::queue<TrackingId>& LogisticsManager::queueFor(const Package& package) {
    return package.serviceType() == ServiceType::Express ? expressQueue_ : regularQueue_;
}

std::queue<TrackingId>* LogisticsManager::queueOfNextWaiting() {
    // Express queue first; the regular one only when no express shipment is waiting.
    for (std::queue<TrackingId>* queue : {&expressQueue_, &regularQueue_}) {
        while (!queue->empty()) {
            const auto it = packages_.find(queue->front());
            if (it != packages_.end() && it->second->getStatus() == PackageStatus::Accepted) {
                return queue;
            }
            queue->pop();
        }
    }
    return nullptr;
}

// Adding the record first means the only step left after the change is popping it on failure,
// and pop_back cannot throw.
void LogisticsManager::recordAndRun(OperationRecord record, const std::function<void()>& change) {
    history_.push_back(std::move(record));
    try {
        change();
    } catch (...) {
        history_.pop_back();
        throw;
    }
}

void LogisticsManager::run(OperationType type, std::unique_ptr<ShipmentCommand> command) {
    OperationRecord record(type, command->getId(), command->describe());
    recordAndRun(std::move(record), [this, &command] { commands_.execute(std::move(command)); });
}

}  // namespace courier
