#include "courier/Assignee.hpp"

#include <format>
#include <utility>

#include "Validation.hpp"

namespace courier {

Assignee::Assignee(std::string code, std::string name)
    : code_(requireCode(std::move(code))), name_(requireNotBlank("name", std::move(name))) {}

// Defined here, not in the header, so the class has a key function and its vtable is emitted once.
Assignee::~Assignee() = default;

const std::string& Assignee::getCode() const { return code_; }

const std::string& Assignee::getName() const { return name_; }

void Assignee::setName(std::string name) { name_ = requireNotBlank("name", std::move(name)); }

// Template Method: the common part is built here, each kind adds its own contact details.
std::string Assignee::describe() const {
    std::string text = std::format("{} {}: {}", toString(kind()), code_, name_);
    if (const std::string contact = contactDetails(); !contact.empty()) {
        text += std::format(" ({})", contact);
    }
    return text;
}

std::ostream& operator<<(std::ostream& out, const Assignee& assignee) {
    return out << assignee.describe();
}

}  // namespace courier
