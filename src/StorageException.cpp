#include "courier/StorageException.hpp"

#include <format>

namespace courier {

StorageException::StorageException(const std::string& message) : LogisticsException(message) {}

StorageException::StorageException(const std::string& source, std::size_t line,
                                   const std::string& reason)
    : LogisticsException(std::format("{} line {}: {}", source, line, reason)) {}

}  // namespace courier
