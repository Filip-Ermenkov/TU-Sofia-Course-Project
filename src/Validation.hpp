#pragma once

#include <string>

// Internal helpers shared by the classes that validate their input.
// Lives in src/ because it is not part of the library's public interface.
namespace courier {

// Returns value if it is finite and greater than zero, otherwise throws ValidationException.
double requirePositive(const std::string& field, double value);

}  // namespace courier
