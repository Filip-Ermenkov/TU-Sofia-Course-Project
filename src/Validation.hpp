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

// Returns code if it has 1 to Assignee::kMaxCodeLength characters, each of them A-Z, 0-9 or '-';
// otherwise throws ValidationException. Keeps codes short to type and safe to store in a file.
std::string requireCode(std::string code);

// The shipment rules, shared by Package and the classes that prepare changes to it.
// Weight must be in (0, Package::kMaxWeightKg]; declared value in [0, Package::kMaxDeclaredValue].
double requireWeight(double weightKg);
Money requireDeclaredValue(Money value);

}  // namespace courier
