#include <cstdlib>
#include <exception>
#include <format>
#include <initializer_list>
#include <iostream>

#include "courier/LogisticsException.hpp"
#include "courier/LogisticsManager.hpp"
#include "courier/PackageNotFoundException.hpp"

namespace {

void printSummary(const courier::Package& package) {
    std::cout << std::format("  {}  {:<9}{:<12}{:<24}{:>10}\n", package.getId().getValue(),
                             toString(package.serviceType()), toString(package.getStatus()),
                             package.getDestination(), package.totalCost().toString());
}

}  // namespace

int main() {
    using courier::Dimensions;
    using courier::Money;
    using courier::PackageStatus;
    using courier::ServiceType;
    using courier::TrackingId;

    std::cout << "Courier Shipment Management System\n\n";
    try {
        courier::LogisticsManager manager;
        const Dimensions box(30.0, 20.0, 10.0);
        manager.accept(ServiceType::Standard, 2.0, box, "Ivan Petrov", "Sofia, 1 Vitosha Blvd");
        manager.accept(ServiceType::Fragile, 1.5, box, "Maria Ivanova", "Plovdiv, 5 Main St",
                       Money(20000));
        const auto& express = manager.accept(ServiceType::Express, 3.0, box, "Georgi Dimitrov",
                                             "Varna, 10 Sea Garden", Money(10000));
        manager.accept(ServiceType::Standard, 8.0, Dimensions(60.0, 40.0, 40.0), "Elena Koleva",
                       "Ruse, 3 Danube St");
        std::cout << express << '\n';

        // Express goes first, then the rest in arrival order.
        for (int i = 0; i < 2; ++i) {
            if (const auto id = manager.dispatchNext()) {
                std::cout << "Dispatched " << id->getValue() << '\n';
            }
        }
        manager.changeStatus(express.getId(), PackageStatus::Delivered);

        std::cout << "\nSearch results:\n";
        for (const TrackingId id : {TrackingId(2), TrackingId(99)}) {
            try {
                printSummary(manager.get(id));
            } catch (const courier::PackageNotFoundException& error) {
                std::cout << "  " << error.what() << '\n';
            }
        }

        std::cout << "\nUndelivered shipments:\n";
        for (const courier::Package* package : manager.undelivered()) {
            printSummary(*package);
        }

        const Money threshold(1500);
        std::cout << "\nShipments costing more than " << threshold << ":\n";
        const auto expensive = manager.filter([threshold](const courier::Package& package) {
            return package.totalCost() > threshold;
        });
        for (const courier::Package* package : expensive) {
            printSummary(*package);
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
