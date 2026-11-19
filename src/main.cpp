#include <cstdlib>
#include <exception>
#include <format>
#include <initializer_list>
#include <iostream>
#include <memory>

#include "courier/Assignee.hpp"
#include "courier/Courier.hpp"
#include "courier/DispatchModule.hpp"
#include "courier/LogisticsException.hpp"
#include "courier/LogisticsManager.hpp"
#include "courier/Office.hpp"
#include "courier/OperationNotAllowedException.hpp"
#include "courier/OperationRecord.hpp"
#include "courier/PackageNotFoundException.hpp"
#include "courier/ParcelDetails.hpp"

namespace {

void printSummary(const courier::Package& package) {
    std::cout << std::format("  {}  {:<9}{:<12}{:<24}{:>10}\n", package.getId().getValue(),
                             toString(package.serviceType()), toString(package.getStatus()),
                             package.getDestination(), package.totalCost().toString());
}

void printRecord(const courier::OperationRecord& record) {
    std::cout << std::format("  {:<15}{}  {}\n", toString(record.getType()),
                             record.getId().getValue(), record.getDetails());
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
        // Declared first, so it is destroyed after the manager that points to it.
        courier::DispatchModule dispatch(std::cout);
        courier::LogisticsManager manager;
        manager.attach(dispatch);
        const Dimensions box(30.0, 20.0, 10.0);
        const auto& standard =
            manager.accept(ServiceType::Standard, 2.0, box, "Ivan Petrov", "Sofia, 1 Vitosha Blvd");
        const auto& fragile = manager.accept(ServiceType::Fragile, 1.5, box, "Maria Ivanova",
                                             "Plovdiv, 5 Main St", Money(20000));
        const auto& express = manager.accept(ServiceType::Express, 3.0, box, "Georgi Dimitrov",
                                             "Varna, 10 Sea Garden", Money(10000));
        const auto& large = manager.accept(ServiceType::Standard, 8.0, Dimensions(60.0, 40.0, 40.0),
                                           "Elena Koleva", "Ruse, 3 Danube St");

        // Couriers deliver to the door; at an office the recipient collects the shipment.
        manager.addAssignee(
            std::make_unique<courier::Courier>("C-01", "Petar Stoyanov", "0888 123 456"));
        manager.addAssignee(std::make_unique<courier::Courier>("C-02", "Nikolay Todorov"));
        manager.addAssignee(
            std::make_unique<courier::Office>("PLD-1", "Plovdiv Center", "12 Ivan Vazov St"));
        manager.assign(standard.getId(), "C-01");
        manager.assign(express.getId(), "C-02");
        manager.assign(fragile.getId(), "C-02");
        manager.assign(fragile.getId(), "PLD-1");  // the recipient will collect it
        manager.assign(large.getId(), "C-01");
        manager.undo();  // assigned by mistake
        std::cout << express << '\n';

        // Customer requests go through commands, so each one can be undone and redone.
        manager.reroute(standard.getId(), "Sofia, 25 Tsarigradsko Shose");
        manager.undo();
        manager.redo();
        manager.modify(large.getId(), courier::ParcelDetails(9.5, Dimensions(60.0, 40.0, 40.0)));
        manager.undo();
        std::cout << std::format("{} now goes to {}\n\n", standard.getId().getValue(),
                                 standard.getDestination());

        // Express goes first, then the rest in arrival order. The dispatch module reports the
        // express and fragile ones as they change status.
        for (int i = 0; i < 3; ++i) {
            if (const auto id = manager.dispatchNext()) {
                std::cout << "Dispatched " << id->getValue() << '\n';
            }
        }
        manager.changeStatus(express.getId(), PackageStatus::Delivered);
        try {
            manager.reroute(express.getId(), "Burgas, 2 Port St");
        } catch (const courier::OperationNotAllowedException& error) {
            std::cout << "Reroute refused: " << error.what() << '\n';
        }

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

        std::cout << "\nCouriers and offices:\n";
        for (const courier::Assignee* assignee : manager.assignees()) {
            std::cout << "  " << *assignee << '\n';
            for (const courier::Package* package : manager.assignedTo(assignee->getCode())) {
                printSummary(*package);
            }
        }

        std::cout << "\nLogistics history:\n";
        for (const courier::OperationRecord& record : manager.history()) {
            printRecord(record);
        }
        std::cout << "\nCancelled operations:\n";
        for (const courier::OperationRecord& record : manager.cancelledOperations()) {
            printRecord(record);
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
