#include "courier/LogisticsManager.hpp"

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

}  // namespace courier
