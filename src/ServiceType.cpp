#include "courier/ServiceType.hpp"

#include <stdexcept>

namespace courier {

std::string_view toString(ServiceType type) {
    switch (type) {
        case ServiceType::Standard:
            return "Standard";
        case ServiceType::Express:
            return "Express";
        case ServiceType::Fragile:
            return "Fragile";
    }
    // Only reachable if an invalid value was forced in with static_cast.
    throw std::logic_error("Unknown ServiceType");
}

}  // namespace courier
