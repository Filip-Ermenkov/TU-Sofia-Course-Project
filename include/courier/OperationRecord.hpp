#pragma once

#include <chrono>
#include <string>

#include "courier/OperationType.hpp"
#include "courier/TrackingId.hpp"

namespace courier {

// One entry in the logistics history: what was done to which shipment, and when.
// Immutable value type.
class OperationRecord {
public:
    // Stamped with the current time.
    OperationRecord(OperationType type, TrackingId id, std::string details);
    // With a given time, e.g. an entry read back from a saved history.
    OperationRecord(OperationType type, TrackingId id, std::string details,
                    std::chrono::sys_seconds time);

    [[nodiscard]] OperationType getType() const;
    [[nodiscard]] const TrackingId& getId() const;
    [[nodiscard]] const std::string& getDetails() const;
    [[nodiscard]] std::chrono::sys_seconds getTime() const;

private:
    OperationType type_;
    TrackingId id_;
    std::string details_;
    std::chrono::sys_seconds time_;  // UTC, whole seconds
};

}  // namespace courier
