#include "courier/PackageStatus.hpp"

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

}  // namespace courier
