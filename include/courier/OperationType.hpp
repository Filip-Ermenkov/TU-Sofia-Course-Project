#pragma once

#include <string_view>

namespace courier {

// Kind of entry in the logistics history.
enum class OperationType { Accept, StatusChange, Reroute, Modify, Assign, Undo, Redo };

[[nodiscard]] std::string_view toString(OperationType type);

}  // namespace courier
