#pragma once

#include <cstddef>
#include <ostream>
#include <string>

#include "courier/AssigneeKind.hpp"

namespace courier {

// Someone a shipment can be assigned to: a courier who delivers it or an office where it is
// collected. Each one is known by a short code chosen by the company, such as "C-01" or "VAR-1".
class Assignee {
public:
    static constexpr std::size_t kMaxCodeLength = 10;

    virtual ~Assignee();

    // The code never changes: it is the key under which the assignee is registered.
    [[nodiscard]] const std::string& getCode() const;
    [[nodiscard]] const std::string& getName() const;
    void setName(std::string name);

    [[nodiscard]] virtual AssigneeKind kind() const = 0;
    // One line for lists, e.g. "Courier C-01: Ivan Petrov (phone 0888 123 456)".
    [[nodiscard]] std::string describe() const;

protected:
    // The code must be 1 to kMaxCodeLength characters of A-Z, 0-9 and '-'; the name must not be
    // blank. Otherwise throws ValidationException.
    Assignee(std::string code, std::string name);

    // Protected so an Assignee cannot be copied or assigned through a base reference (slicing).
    Assignee(const Assignee&) = default;
    Assignee& operator=(const Assignee&) = default;
    Assignee(Assignee&&) = default;
    Assignee& operator=(Assignee&&) = default;

private:
    // How to reach the assignee, for describe(); empty if nothing was given.
    [[nodiscard]] virtual std::string contactDetails() const = 0;

    std::string code_;
    std::string name_;
};

// Prints describe().
std::ostream& operator<<(std::ostream& out, const Assignee& assignee);

}  // namespace courier
