#include "courier/ServiceType.hpp"

#include <initializer_list>
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

std::optional<ServiceType> parseServiceType(std::string_view text) {
    // Compares with toString(), so each name is written in one place only.
    for (const ServiceType type :
         {ServiceType::Standard, ServiceType::Express, ServiceType::Fragile}) {
        if (toString(type) == text) {
            return type;
        }
    }
    return std::nullopt;
}

}  // namespace courier
