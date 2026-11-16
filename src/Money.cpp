#include "courier/Money.hpp"

#include <cmath>
#include <cstdlib>
#include <format>

namespace courier {

std::string Money::toString() const {
    // Split before taking the absolute value, so even the most negative amount cannot overflow.
    const std::int64_t whole = std::abs(cents_ / 100);
    const std::int64_t rest = std::abs(cents_ % 100);
    return std::format("{}{}.{:02}", cents_ < 0 ? "-" : "", whole, rest);
}

Money Money::multipliedBy(double factor) const {
    return Money(static_cast<std::int64_t>(std::llround(static_cast<double>(cents_) * factor)));
}

std::ostream& operator<<(std::ostream& out, const Money& money) { return out << money.toString(); }

}  // namespace courier
