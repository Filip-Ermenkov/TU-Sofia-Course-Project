#pragma once

#include <string>

// Internal helpers shared by the classes that validate their input.
// Lives in src/ because it is not part of the library's public interface.
namespace courier {

// Returns value if it is finite, greater than zero and at most max; otherwise throws
// ValidationException. The upper limit keeps later arithmetic (volume, prices) far from overflow.
double requireInRange(const std::string& field, double value, double max);

// Returns text if it contains anything besides whitespace, otherwise throws ValidationException.
std::string requireNotBlank(const std::string& field, std::string text);

}  // namespace courier
