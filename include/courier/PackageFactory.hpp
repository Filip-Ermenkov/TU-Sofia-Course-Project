#pragma once

#include <memory>
#include <optional>
#include <string>

#include "courier/Dimensions.hpp"
#include "courier/Money.hpp"
#include "courier/Package.hpp"
#include "courier/ServiceType.hpp"
#include "courier/TrackingId.hpp"

namespace courier {

// The single place where packages are created (Factory pattern). The caller names a service type
// and gets back the matching Package subclass, with a fresh tracking ID already assigned.
class PackageFactory {
public:
    // Starts numbering from CR00000001.
    PackageFactory();
    // Continues after an ID that was already handed out, e.g. the highest one loaded from a file.
    explicit PackageFactory(TrackingId lastIssued);

    // A copy would hand out the same IDs a second time.
    PackageFactory(const PackageFactory&) = delete;
    PackageFactory& operator=(const PackageFactory&) = delete;

    // An ID is used up only if the package was created; invalid input throws ValidationException.
    [[nodiscard]] std::unique_ptr<Package> create(ServiceType type, double weightKg,
                                                  Dimensions dimensions, std::string recipient,
                                                  std::string destination, Money declaredValue);
    [[nodiscard]] std::unique_ptr<Package> create(ServiceType type, double weightKg,
                                                  Dimensions dimensions, std::string recipient,
                                                  std::string destination);

    // Empty until the first package is created.
    [[nodiscard]] std::optional<TrackingId> getLastIssued() const;

private:
    [[nodiscard]] TrackingId nextId() const;

    std::optional<TrackingId> lastIssued_;
};

}  // namespace courier
