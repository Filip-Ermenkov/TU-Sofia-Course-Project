#include "courier/OperationRecord.hpp"

#include <utility>

#include "Validation.hpp"

namespace courier {

OperationRecord::OperationRecord(OperationType type, TrackingId id, std::string details)
    : OperationRecord(type, id, std::move(details),
                      std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now())) {}

OperationRecord::OperationRecord(OperationType type, TrackingId id, std::string details,
                                 std::chrono::sys_seconds time)
    : type_(type), id_(id), details_(requireNotBlank("details", std::move(details))), time_(time) {}

OperationType OperationRecord::getType() const { return type_; }

const TrackingId& OperationRecord::getId() const { return id_; }

const std::string& OperationRecord::getDetails() const { return details_; }

std::chrono::sys_seconds OperationRecord::getTime() const { return time_; }

}  // namespace courier
