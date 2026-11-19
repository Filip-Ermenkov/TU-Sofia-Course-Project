#pragma once

#include <string>

#include "courier/Money.hpp"

// Internal helpers shared by the classes that validate their input.
// Lives in src/ because it is not part of the library's public interface.
namespace courier {

// Returns value if it is finite, greater than zero and at most max; otherwise throws
// ValidationException. The upper limit keeps later arithmetic (volume, prices) far from overflow.
double requireInRange(const std::string& field, double value, double max);

// Returns text if it contains anything besides whitespace, otherwise throws ValidationException.
std::string requireNotBlank(const std::string& field, std::string text);

// The shipment rules, shared by Package and the classes that prepare changes to it.
// Weight must be in (0, Package::kMaxWeightKg]; declared value in [0, Package::kMaxDeclaredValue].
double requireWeight(double weightKg);
Money requireDeclaredValue(Money value);

}  // namespace courier
