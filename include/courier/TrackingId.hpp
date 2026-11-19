#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
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

// Lets TrackingId be the key of std::unordered_map and std::unordered_set. Two equal IDs have the
// same number, so they get the same hash, which is the one rule a hash must follow.
namespace std {

template <>
struct hash<courier::TrackingId> {
    size_t operator()(const courier::TrackingId& id) const noexcept {
        return hash<uint32_t>{}(id.getNumber());
    }
};

}  // namespace std
