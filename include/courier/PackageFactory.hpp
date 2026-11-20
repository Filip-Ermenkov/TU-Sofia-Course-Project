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

    // A copy would hand out the same IDs a second time, and a moved-from factory would still hold
    // its counter, so neither is allowed. Written out in full so the intent is visible (rule of
    // five).
    PackageFactory(const PackageFactory&) = delete;
    PackageFactory& operator=(const PackageFactory&) = delete;
    PackageFactory(PackageFactory&&) = delete;
    PackageFactory& operator=(PackageFactory&&) = delete;
    ~PackageFactory() = default;

    // An ID is used up only if the package was created; invalid input throws ValidationException.
    [[nodiscard]] std::unique_ptr<Package> create(ServiceType type, double weightKg,
                                                  Dimensions dimensions, std::string recipient,
                                                  std::string destination, Money declaredValue);
    [[nodiscard]] std::unique_ptr<Package> create(ServiceType type, double weightKg,
                                                  Dimensions dimensions, std::string recipient,
                                                  std::string destination);

    // Re-creates a package that was issued earlier, e.g. one read back from a file, with the ID it
    // already has. The counter moves up to at least that ID, so create() never issues it again.
    // The caller makes sure no other package has the ID. Invalid input throws ValidationException
    // and changes nothing.
    [[nodiscard]] std::unique_ptr<Package> restore(ServiceType type, TrackingId id, double weightKg,
                                                   Dimensions dimensions, std::string recipient,
                                                   std::string destination, Money declaredValue);

    // Empty until the first package is created.
    [[nodiscard]] std::optional<TrackingId> getLastIssued() const;

private:
    [[nodiscard]] TrackingId nextId() const;

    std::optional<TrackingId> lastIssued_;
};

}  // namespace courier
