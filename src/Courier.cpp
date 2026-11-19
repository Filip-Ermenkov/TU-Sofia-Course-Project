#include "courier/Courier.hpp"

#include <utility>

#include "Validation.hpp"

namespace courier {

Courier::Courier(std::string code, std::string name) : Assignee(std::move(code), std::move(name)) {}

Courier::Courier(std::string code, std::string name, std::string phone)
    : Assignee(std::move(code), std::move(name)),
      phone_(requireNotBlank("phone", std::move(phone))) {}

const std::string& Courier::getPhone() const { return phone_; }

void Courier::setPhone(std::string phone) { phone_ = requireNotBlank("phone", std::move(phone)); }

AssigneeKind Courier::kind() const { return AssigneeKind::Courier; }

std::string Courier::contactDetails() const { return phone_.empty() ? "" : "phone " + phone_; }

}  // namespace courier
