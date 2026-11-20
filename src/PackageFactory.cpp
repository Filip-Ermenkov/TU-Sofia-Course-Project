#include "courier/PackageFactory.hpp"

#include <stdexcept>
#include <utility>

#include "courier/ExpressPackage.hpp"
#include "courier/FragilePackage.hpp"
#include "courier/LogisticsException.hpp"
#include "courier/StandardPackage.hpp"

namespace courier {

namespace {

std::unique_ptr<Package> makePackage(ServiceType type, TrackingId id, double weightKg,
                                     Dimensions dimensions, std::string recipient,
                                     std::string destination, Money declaredValue) {
    switch (type) {
        case ServiceType::Standard:
            return std::make_unique<StandardPackage>(id, weightKg, dimensions, std::move(recipient),
                                                     std::move(destination), declaredValue);
        case ServiceType::Express:
            return std::make_unique<ExpressPackage>(id, weightKg, dimensions, std::move(recipient),
                                                    std::move(destination), declaredValue);
        case ServiceType::Fragile:
            return std::make_unique<FragilePackage>(id, weightKg, dimensions, std::move(recipient),
                                                    std::move(destination), declaredValue);
    }
    // Only reachable if an invalid value was forced in with static_cast.
    throw std::logic_error("Unknown ServiceType");
}

}  // namespace

PackageFactory::PackageFactory() = default;

PackageFactory::PackageFactory(TrackingId lastIssued) : lastIssued_(lastIssued) {}

std::unique_ptr<Package> PackageFactory::create(ServiceType type, double weightKg,
                                                Dimensions dimensions, std::string recipient,
                                                std::string destination, Money declaredValue) {
    const TrackingId id = nextId();
    auto package = makePackage(type, id, weightKg, dimensions, std::move(recipient),
                               std::move(destination), declaredValue);
    lastIssued_ = id;  // only after the package was built, so a failed attempt skips no number
    return package;
}

std::unique_ptr<Package> PackageFactory::create(ServiceType type, double weightKg,
                                                Dimensions dimensions, std::string recipient,
                                                std::string destination) {
    return create(type, weightKg, dimensions, std::move(recipient), std::move(destination),
                  Money());
}

std::unique_ptr<Package> PackageFactory::restore(ServiceType type, TrackingId id, double weightKg,
                                                 Dimensions dimensions, std::string recipient,
                                                 std::string destination, Money declaredValue) {
    auto package = makePackage(type, id, weightKg, dimensions, std::move(recipient),
                               std::move(destination), declaredValue);
    if (!lastIssued_ || id.getNumber() > lastIssued_->getNumber()) {
        lastIssued_ = id;
    }
    return package;
}

std::optional<TrackingId> PackageFactory::getLastIssued() const { return lastIssued_; }

TrackingId PackageFactory::nextId() const {
    const std::uint32_t last = lastIssued_ ? lastIssued_->getNumber() : 0;
    if (last == TrackingId::kMaxNumber) {
        throw LogisticsException("No tracking IDs left to issue");
    }
    return TrackingId(last + 1);
}

}  // namespace courier
