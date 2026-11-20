#pragma once

#include <optional>
#include <string_view>

namespace courier {

// Where a shipment is in the delivery process. It only ever moves forward, one step at a time.
enum class PackageStatus { Accepted, InTransit, Delivered };

[[nodiscard]] std::string_view toString(PackageStatus status);
// The value whose toString() text this is; empty if the text matches none, e.g. a damaged file.
[[nodiscard]] std::optional<PackageStatus> parsePackageStatus(std::string_view text);

// True if a shipment may move directly from `from` to `to`.
[[nodiscard]] bool canTransition(PackageStatus from, PackageStatus to);

}  // namespace courier
