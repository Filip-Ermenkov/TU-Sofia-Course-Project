#include "courier/LogisticsManager.hpp"

#include <algorithm>
#include <initializer_list>
#include <ranges>
#include <utility>

#include "courier/PackageNotFoundException.hpp"

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
    try {
        queueFor(*it->second).push(id);
    } catch (...) {
        packages_.erase(it);  // out of memory: undo the registration, so nothing half-done stays
        throw;
    }
    return *it->second;
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
    package.setStatus(newStatus);  // throws before anyone is told if the step is not allowed
    for (StatusObserver* observer : observers_) {
        observer->onStatusChanged(package, previous);
    }
}

std::optional<TrackingId> LogisticsManager::dispatchNext() {
    const auto next = takeNextWaiting();
    if (next) {
        changeStatus(*next, PackageStatus::InTransit);
    }
    return next;
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

std::optional<TrackingId> LogisticsManager::takeNextWaiting() {
    // Express queue first; the regular one only when no express shipment is waiting.
    for (std::queue<TrackingId>* queue : {&expressQueue_, &regularQueue_}) {
        while (!queue->empty()) {
            const TrackingId id = queue->front();
            queue->pop();
            if (get(id).getStatus() == PackageStatus::Accepted) {
                return id;
            }
        }
    }
    return std::nullopt;
}

}  // namespace courier
