#pragma once

#include <string_view>

namespace courier {

// Kind of place or person a shipment can be assigned to.
enum class AssigneeKind { Courier, Office };

[[nodiscard]] std::string_view toString(AssigneeKind kind);

}  // namespace courier
