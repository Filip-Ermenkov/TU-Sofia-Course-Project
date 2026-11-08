#pragma once

#include <string_view>

namespace courier {

// Where a shipment is in the delivery process. It only ever moves forward, one step at a time.
enum class PackageStatus { Accepted, InTransit, Delivered };

[[nodiscard]] std::string_view toString(PackageStatus status);

// True if a shipment may move directly from `from` to `to`.
[[nodiscard]] bool canTransition(PackageStatus from, PackageStatus to);

}  // namespace courier
