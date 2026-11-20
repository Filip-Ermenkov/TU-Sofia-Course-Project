#include "courier/PackageStatus.hpp"

#include <initializer_list>
#include <stdexcept>

namespace courier {

std::string_view toString(PackageStatus status) {
    switch (status) {
        case PackageStatus::Accepted:
            return "Accepted";
        case PackageStatus::InTransit:
            return "In transit";
        case PackageStatus::Delivered:
            return "Delivered";
    }
    throw std::logic_error("Unknown PackageStatus");
}

bool canTransition(PackageStatus from, PackageStatus to) {
    switch (from) {
        case PackageStatus::Accepted:
            return to == PackageStatus::InTransit;
        case PackageStatus::InTransit:
            return to == PackageStatus::Delivered;
        case PackageStatus::Delivered:
            return false;
    }
    throw std::logic_error("Unknown PackageStatus");
}

std::optional<PackageStatus> parsePackageStatus(std::string_view text) {
    // Compares with toString(), so each name is written in one place only.
    for (const PackageStatus status :
         {PackageStatus::Accepted, PackageStatus::InTransit, PackageStatus::Delivered}) {
        if (toString(status) == text) {
            return status;
        }
    }
    return std::nullopt;
}

}  // namespace courier
