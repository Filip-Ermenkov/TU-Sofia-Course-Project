#include "courier/LogisticsManager.hpp"

#include <algorithm>
#include <initializer_list>
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

void LogisticsManager::changeStatus(const TrackingId& id, PackageStatus newStatus) {
    findPackage(id).setStatus(newStatus);
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
