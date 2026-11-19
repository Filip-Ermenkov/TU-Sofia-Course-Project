#pragma once

#include <string>

#include "courier/Assignee.hpp"
#include "courier/AssigneeKind.hpp"

namespace courier {

// A courier office where the recipient collects the shipment.
class Office final : public Assignee {
public:
    // Without an address.
    Office(std::string code, std::string name);
    Office(std::string code, std::string name, std::string address);

    // Empty if no address was given.
    [[nodiscard]] const std::string& getAddress() const;
    void setAddress(std::string address);

    [[nodiscard]] AssigneeKind kind() const override;

private:
    [[nodiscard]] std::string contactDetails() const override;

    std::string address_;
};

}  // namespace courier
