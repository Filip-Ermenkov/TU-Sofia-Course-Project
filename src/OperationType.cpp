#include "courier/OperationType.hpp"

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
        case OperationType::Undo:
            return "Undo";
        case OperationType::Redo:
            return "Redo";
    }
    // Only reachable if an invalid value was forced in with static_cast.
    throw std::logic_error("Unknown OperationType");
}

}  // namespace courier
