#pragma once

#include <optional>
#include <string_view>

namespace courier {

// Kind of entry in the logistics history.
enum class OperationType { Accept, StatusChange, Reroute, Modify, Assign, Undo, Redo };

[[nodiscard]] std::string_view toString(OperationType type);
// The value whose toString() text this is; empty if the text matches none, e.g. a damaged file.
[[nodiscard]] std::optional<OperationType> parseOperationType(std::string_view text);

}  // namespace courier
