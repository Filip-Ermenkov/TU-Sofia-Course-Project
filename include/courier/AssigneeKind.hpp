#pragma once

#include <optional>
#include <string_view>

namespace courier {

// Kind of place or person a shipment can be assigned to.
enum class AssigneeKind { Courier, Office };

[[nodiscard]] std::string_view toString(AssigneeKind kind);
// The value whose toString() text this is; empty if the text matches none, e.g. a damaged file.
[[nodiscard]] std::optional<AssigneeKind> parseAssigneeKind(std::string_view text);

}  // namespace courier
