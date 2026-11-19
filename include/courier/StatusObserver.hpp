#pragma once

#include "courier/Package.hpp"
#include "courier/PackageStatus.hpp"

namespace courier {

// Observer pattern: something that wants to hear about every status change made through the
// LogisticsManager. The manager does not own its observers, so an observer must stay alive while
// it is attached.
class StatusObserver {
public:
    virtual ~StatusObserver();

    // Called after the change has been made: package.getStatus() is already the new status.
    // noexcept because the change cannot be undone any more; a failing observer must not make the
    // caller believe it did not happen. Must not attach or detach observers.
    virtual void onStatusChanged(const Package& package, PackageStatus previous) noexcept = 0;

protected:
    // Protected: only subclasses create observers, and copying through a base reference
    // (slicing) is not possible.
    StatusObserver() = default;
    StatusObserver(const StatusObserver&) = default;
    StatusObserver& operator=(const StatusObserver&) = default;
    StatusObserver(StatusObserver&&) = default;
    StatusObserver& operator=(StatusObserver&&) = default;
};

}  // namespace courier
