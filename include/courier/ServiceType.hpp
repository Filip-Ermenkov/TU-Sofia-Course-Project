#pragma once

#include <string_view>

namespace courier {

// Delivery service a customer can order. Decides which kind of package is created.
enum class ServiceType { Standard, Express, Fragile };

[[nodiscard]] std::string_view toString(ServiceType type);

}  // namespace courier
