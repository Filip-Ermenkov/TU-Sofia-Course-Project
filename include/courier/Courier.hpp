#pragma once

#include <string>

#include "courier/Assignee.hpp"
#include "courier/AssigneeKind.hpp"

namespace courier {

// A courier who takes shipments to the recipient's door.
class Courier final : public Assignee {
public:
    // Without a phone number.
    Courier(std::string code, std::string name);
    Courier(std::string code, std::string name, std::string phone);

    // Empty if no phone number was given.
    [[nodiscard]] const std::string& getPhone() const;
    void setPhone(std::string phone);

    [[nodiscard]] AssigneeKind kind() const override;

private:
    [[nodiscard]] std::string contactDetails() const override;

    std::string phone_;
};

}  // namespace courier
