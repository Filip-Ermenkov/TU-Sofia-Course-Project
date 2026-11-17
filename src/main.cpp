#include <array>
#include <cstdlib>
#include <exception>
#include <iostream>

#include "courier/LogisticsException.hpp"
#include "courier/PackageFactory.hpp"

int main() {
    using courier::ServiceType;

    std::cout << "Courier Shipment Management System\n\n";
    try {
        courier::PackageFactory factory;
        const courier::Dimensions box(30.0, 20.0, 10.0);
        constexpr std::array kServices{ServiceType::Standard, ServiceType::Express,
                                       ServiceType::Fragile};
        for (const ServiceType service : kServices) {
            const auto package = factory.create(service, 2.0, box, "Ivan Petrov",
                                                "Sofia, 1 Vitosha Blvd", courier::Money(10000));
            std::cout << *package << '\n';
        }
    } catch (const courier::LogisticsException& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return EXIT_FAILURE;
    } catch (const std::exception& error) {
        std::cerr << "Unexpected error: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
