#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace courier {

// Shipment number shown to customers: "CR" followed by 8 digits, e.g. CR00000042.
// Immutable: a shipment never changes its ID, so there are no setters.
class TrackingId {
public:
    static constexpr std::uint32_t kMaxNumber = 99'999'999;

    explicit TrackingId(std::uint32_t number);
    explicit TrackingId(std::string_view text);

    [[nodiscard]] std::uint32_t getNumber() const;
    [[nodiscard]] std::string getValue() const;

    bool operator==(const TrackingId& other) const = default;

private:
    std::uint32_t number_;
};

}  // namespace courier
