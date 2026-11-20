#pragma once

#include <optional>
#include <string_view>

namespace courier {

// Delivery service a customer can order. Decides which kind of package is created.
enum class ServiceType { Standard, Express, Fragile };

[[nodiscard]] std::string_view toString(ServiceType type);
// The value whose toString() text this is; empty if the text matches none, e.g. a damaged file.
[[nodiscard]] std::optional<ServiceType> parseServiceType(std::string_view text);

}  // namespace courier
