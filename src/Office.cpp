#include "courier/Office.hpp"

#include <utility>

#include "Validation.hpp"

namespace courier {

Office::Office(std::string code, std::string name) : Assignee(std::move(code), std::move(name)) {}

Office::Office(std::string code, std::string name, std::string address)
    : Assignee(std::move(code), std::move(name)),
      address_(requireNotBlank("address", std::move(address))) {}

const std::string& Office::getAddress() const { return address_; }

void Office::setAddress(std::string address) {
    address_ = requireNotBlank("address", std::move(address));
}

AssigneeKind Office::kind() const { return AssigneeKind::Office; }

std::string Office::contactDetails() const { return address_; }

}  // namespace courier
