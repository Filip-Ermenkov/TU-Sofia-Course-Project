#include "courier/AssigneeKind.hpp"

#include <initializer_list>
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

std::optional<AssigneeKind> parseAssigneeKind(std::string_view text) {
    // Compares with toString(), so each name is written in one place only.
    for (const AssigneeKind kind : {AssigneeKind::Courier, AssigneeKind::Office}) {
        if (toString(kind) == text) {
            return kind;
        }
    }
    return std::nullopt;
}

}  // namespace courier
