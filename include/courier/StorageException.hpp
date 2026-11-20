#pragma once

#include <cstddef>
#include <string>

#include "courier/LogisticsException.hpp"

namespace courier {

// Thrown when saved data cannot be written or read back, e.g. a missing or damaged file.
class StorageException : public LogisticsException {
public:
    explicit StorageException(const std::string& message);
    // Points to the place in a file where reading failed, e.g. "shipments.csv line 4: ...".
    StorageException(const std::string& source, std::size_t line, const std::string& reason);
};

}  // namespace courier
