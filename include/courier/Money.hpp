#pragma once

#include <compare>
#include <cstdint>
#include <ostream>
#include <string>

namespace courier {

// An amount of money stored as whole cents, so adding prices is always exact.
// Immutable value type: operations return a new Money instead of changing this one.
class Money {
public:
    constexpr Money() = default;
    constexpr explicit Money(std::int64_t cents) : cents_(cents) {}

    [[nodiscard]] constexpr std::int64_t getCents() const { return cents_; }

    // Formatted with two decimals, e.g. "9.00" or "-0.50".
    [[nodiscard]] std::string toString() const;

    // The only place where a fractional result is rounded back to whole cents.
    [[nodiscard]] Money multipliedBy(double factor) const;

    constexpr Money operator+(const Money& other) const { return Money(cents_ + other.cents_); }
    auto operator<=>(const Money& other) const = default;

private:
    std::int64_t cents_ = 0;
};

std::ostream& operator<<(std::ostream& out, const Money& money);

}  // namespace courier
