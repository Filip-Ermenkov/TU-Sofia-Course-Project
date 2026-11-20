#include "courier/OperationType.hpp"

#include <initializer_list>
#include <stdexcept>

namespace courier {

std::string_view toString(OperationType type) {
    switch (type) {
        case OperationType::Accept:
            return "Accept";
        case OperationType::StatusChange:
            return "Status change";
        case OperationType::Reroute:
            return "Reroute";
        case OperationType::Modify:
            return "Modify";
        case OperationType::Assign:
            return "Assign";
        case OperationType::Undo:
            return "Undo";
        case OperationType::Redo:
            return "Redo";
    }
    // Only reachable if an invalid value was forced in with static_cast.
    throw std::logic_error("Unknown OperationType");
}

std::optional<OperationType> parseOperationType(std::string_view text) {
    // Compares with toString(), so each name is written in one place only.
    for (const OperationType type :
         {OperationType::Accept, OperationType::StatusChange, OperationType::Reroute,
          OperationType::Modify, OperationType::Assign, OperationType::Undo, OperationType::Redo}) {
        if (toString(type) == text) {
            return type;
        }
    }
    return std::nullopt;
}

}  // namespace courier
