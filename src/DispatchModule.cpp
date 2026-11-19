#include "courier/DispatchModule.hpp"

#include <format>

#include "courier/ServiceType.hpp"

namespace courier {

DispatchModule::DispatchModule() : out_(nullptr) {}

DispatchModule::DispatchModule(std::ostream& out) : out_(&out) {}

void DispatchModule::onStatusChanged(const Package& package, PackageStatus previous) noexcept {
    if (!package.isPriority()) {
        return;
    }
    // If memory runs out here, noexcept ends the program (std::terminate) instead of letting the
    // manager's caller think the status change failed.
    alerts_.push_back(std::format("{} ({}): {} -> {}", package.getId().getValue(),
                                  toString(package.serviceType()), toString(previous),
                                  toString(package.getStatus())));
    if (out_ != nullptr) {
        *out_ << "[dispatch] " << alerts_.back() << '\n';
    }
}

const std::vector<std::string>& DispatchModule::alerts() const { return alerts_; }

}  // namespace courier
