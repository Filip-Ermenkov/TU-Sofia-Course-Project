#pragma once

#include <ostream>
#include <string>
#include <vector>

#include "courier/Package.hpp"
#include "courier/PackageStatus.hpp"
#include "courier/StatusObserver.hpp"

namespace courier {

// The dispatch office. Attached to the LogisticsManager as an observer, it is told about every
// status change but only reacts to priority shipments (express and fragile): it records an alert
// for each one and, if given a stream, prints it straight away.
class DispatchModule final : public StatusObserver {
public:
    // Only records the alerts.
    DispatchModule();
    // Also writes every alert to the stream, which must outlive the module.
    explicit DispatchModule(std::ostream& out);

    void onStatusChanged(const Package& package, PackageStatus previous) noexcept override;

    // Alerts in the order they were raised, e.g. "CR00000003 (Express): Accepted -> In transit".
    [[nodiscard]] const std::vector<std::string>& alerts() const;

private:
    std::ostream* out_;  // not owned; null when the alerts are only recorded
    std::vector<std::string> alerts_;
};

}  // namespace courier
