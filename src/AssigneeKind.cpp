#include "courier/AssigneeKind.hpp"

#include <stdexcept>

namespace courier {

std::string_view toString(AssigneeKind kind) {
    switch (kind) {
        case AssigneeKind::Courier:
            return "Courier";
        case AssigneeKind::Office:
            return "Office";
    }
    // Only reachable if an invalid value was forced in with static_cast.
    throw std::logic_error("Unknown AssigneeKind");
}

}  // namespace courier
