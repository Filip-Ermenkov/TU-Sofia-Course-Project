#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>

#include "courier/InvalidStatusChangeException.hpp"
#include "courier/LogisticsManager.hpp"
#include "courier/PackageNotFoundException.hpp"
#include "courier/ValidationException.hpp"

using courier::Dimensions;
using courier::InvalidStatusChangeException;
using courier::LogisticsManager;
using courier::Money;
using courier::Package;
using courier::PackageNotFoundException;
using courier::PackageStatus;
using courier::ServiceType;
using courier::TrackingId;
using courier::ValidationException;

namespace {

const Dimensions kBox(10.0);

}  // namespace

TEST_CASE("LogisticsManager starts empty") {
    const LogisticsManager manager;
    REQUIRE(manager.packageCount() == 0);
    REQUIRE_FALSE(manager.contains(TrackingId(1)));
}

TEST_CASE("LogisticsManager accepts shipments with new tracking IDs") {
    LogisticsManager manager;
    const Package& first = manager.accept(ServiceType::Standard, 1.0, kBox, "Ivan", "Sofia");
    const Package& second =
        manager.accept(ServiceType::Express, 2.0, kBox, "Maria", "Varna", Money(5000));

    REQUIRE(first.getId() == TrackingId(1));
    REQUIRE(second.getId() == TrackingId(2));
    REQUIRE(second.serviceType() == ServiceType::Express);
    REQUIRE(second.getDeclaredValue() == Money(5000));
    REQUIRE(second.getStatus() == PackageStatus::Accepted);
    REQUIRE(manager.packageCount() == 2);
}

TEST_CASE("LogisticsManager finds a shipment by tracking ID") {
    LogisticsManager manager;
    manager.accept(ServiceType::Standard, 1.0, kBox, "Ivan", "Sofia");
    manager.accept(ServiceType::Fragile, 3.0, kBox, "Maria", "Varna");

    REQUIRE(manager.contains(TrackingId("CR00000002")));
    const Package& found = manager.get(TrackingId("CR00000002"));
    REQUIRE(found.getRecipient() == "Maria");
    REQUIRE(found.serviceType() == ServiceType::Fragile);
}

TEST_CASE("LogisticsManager reports an unknown tracking ID") {
    LogisticsManager manager;
    manager.accept(ServiceType::Standard, 1.0, kBox, "Ivan", "Sofia");
    REQUIRE_FALSE(manager.contains(TrackingId(2)));
    REQUIRE_THROWS_AS(manager.get(TrackingId(2)), PackageNotFoundException);
}

TEST_CASE("LogisticsManager registers nothing when the input is invalid") {
    LogisticsManager manager;
    REQUIRE_THROWS_AS(manager.accept(ServiceType::Standard, 0.0, kBox, "Ivan", "Sofia"),
                      ValidationException);
    REQUIRE(manager.packageCount() == 0);

    const Package& package = manager.accept(ServiceType::Standard, 1.0, kBox, "Ivan", "Sofia");
    REQUIRE(package.getId() == TrackingId(1));
}

TEST_CASE("LogisticsManager continues numbering after the last issued ID") {
    LogisticsManager manager(TrackingId(41));
    const Package& package = manager.accept(ServiceType::Standard, 1.0, kBox, "Ivan", "Sofia");
    REQUIRE(package.getId() == TrackingId(42));
}

TEST_CASE("References to shipments stay valid while more are accepted") {
    LogisticsManager manager;
    const Package& first = manager.accept(ServiceType::Standard, 1.0, kBox, "Ivan", "Sofia");
    for (int i = 0; i < 100; ++i) {
        manager.accept(ServiceType::Standard, 1.0, kBox, "Someone", "Ruse");
    }
    REQUIRE(&manager.get(TrackingId(1)) == &first);
    REQUIRE(first.getRecipient() == "Ivan");
}

TEST_CASE("LogisticsManager changes the status of a shipment") {
    LogisticsManager manager;
    const Package& package = manager.accept(ServiceType::Standard, 1.0, kBox, "Ivan", "Sofia");

    manager.changeStatus(TrackingId(1), PackageStatus::InTransit);
    REQUIRE(package.getStatus() == PackageStatus::InTransit);
    manager.changeStatus(TrackingId(1), PackageStatus::Delivered);
    REQUIRE(package.getStatus() == PackageStatus::Delivered);
}

TEST_CASE("LogisticsManager rejects a status change that is not allowed") {
    LogisticsManager manager;
    const Package& package = manager.accept(ServiceType::Standard, 1.0, kBox, "Ivan", "Sofia");

    REQUIRE_THROWS_AS(manager.changeStatus(TrackingId(1), PackageStatus::Delivered),
                      InvalidStatusChangeException);
    REQUIRE(package.getStatus() == PackageStatus::Accepted);
    REQUIRE_THROWS_AS(manager.changeStatus(TrackingId(9), PackageStatus::InTransit),
                      PackageNotFoundException);
}

TEST_CASE("LogisticsManager dispatches nothing when no shipment is waiting") {
    LogisticsManager manager;
    REQUIRE_FALSE(manager.dispatchNext().has_value());
    REQUIRE(manager.waitingCount() == 0);
}

TEST_CASE("LogisticsManager dispatches express shipments first, the rest in arrival order") {
    LogisticsManager manager;
    manager.accept(ServiceType::Standard, 1.0, kBox, "A", "Sofia");  // CR00000001
    manager.accept(ServiceType::Fragile, 1.0, kBox, "B", "Sofia");   // CR00000002
    manager.accept(ServiceType::Express, 1.0, kBox, "C", "Sofia");   // CR00000003
    manager.accept(ServiceType::Standard, 1.0, kBox, "D", "Sofia");  // CR00000004
    manager.accept(ServiceType::Express, 1.0, kBox, "E", "Sofia");   // CR00000005
    REQUIRE(manager.waitingCount() == 5);

    REQUIRE(manager.dispatchNext() == TrackingId(3));
    REQUIRE(manager.dispatchNext() == TrackingId(5));
    REQUIRE(manager.dispatchNext() == TrackingId(1));
    REQUIRE(manager.dispatchNext() == TrackingId(2));
    REQUIRE(manager.dispatchNext() == TrackingId(4));
    REQUIRE_FALSE(manager.dispatchNext().has_value());

    REQUIRE(manager.waitingCount() == 0);
    REQUIRE(manager.get(TrackingId(4)).getStatus() == PackageStatus::InTransit);
}

TEST_CASE("LogisticsManager skips queued shipments that already left") {
    LogisticsManager manager;
    manager.accept(ServiceType::Standard, 1.0, kBox, "A", "Sofia");
    manager.accept(ServiceType::Standard, 1.0, kBox, "B", "Sofia");
    manager.changeStatus(TrackingId(1), PackageStatus::InTransit);
    REQUIRE(manager.waitingCount() == 1);

    REQUIRE(manager.dispatchNext() == TrackingId(2));
    REQUIRE_FALSE(manager.dispatchNext().has_value());
}

TEST_CASE("LogisticsManager filters shipments with a lambda, ordered by tracking ID") {
    LogisticsManager manager;
    for (int i = 0; i < 20; ++i) {
        const auto type = i % 2 == 0 ? ServiceType::Express : ServiceType::Standard;
        manager.accept(type, 1.0, kBox, "Someone", i % 4 == 0 ? "Varna" : "Sofia");
    }

    const std::string city = "Varna";
    const auto toVarna = manager.filter(
        [&city](const Package& package) { return package.getDestination() == city; });

    REQUIRE(toVarna.size() == 5);
    for (std::size_t i = 0; i < toVarna.size(); ++i) {
        REQUIRE(toVarna[i]->getId() == TrackingId(static_cast<std::uint32_t>(4 * i + 1)));
    }
    REQUIRE(manager.filter([](const Package&) { return false; }).empty());
}

TEST_CASE("Filtered shipments can be sorted with a lambda") {
    LogisticsManager manager;
    manager.accept(ServiceType::Standard, 4.0, kBox, "A", "Sofia");  // 5.00 + 6.00
    manager.accept(ServiceType::Express, 1.0, kBox, "B", "Sofia");   // 8.00 + 2.50
    manager.accept(ServiceType::Fragile, 2.0, kBox, "C", "Sofia");   // 5.00 + 3.00 + 4.00

    auto all = manager.filter([](const Package&) { return true; });
    std::ranges::sort(all, [](const Package* left, const Package* right) {
        return left->totalCost() > right->totalCost();
    });

    REQUIRE(all[0]->getId() == TrackingId(3));
    REQUIRE(all[1]->getId() == TrackingId(1));
    REQUIRE(all[2]->getId() == TrackingId(2));
}

TEST_CASE("LogisticsManager reports undelivered shipments, waiting ones first") {
    LogisticsManager manager;
    for (int i = 0; i < 5; ++i) {
        manager.accept(ServiceType::Standard, 1.0, kBox, "Someone", "Sofia");
    }
    manager.changeStatus(TrackingId(2), PackageStatus::InTransit);
    manager.changeStatus(TrackingId(4), PackageStatus::InTransit);
    manager.changeStatus(TrackingId(4), PackageStatus::Delivered);
    manager.changeStatus(TrackingId(5), PackageStatus::InTransit);

    const auto report = manager.undelivered();

    REQUIRE(report.size() == 4);
    REQUIRE(report[0]->getId() == TrackingId(1));
    REQUIRE(report[1]->getId() == TrackingId(3));
    REQUIRE(report[2]->getId() == TrackingId(2));
    REQUIRE(report[3]->getId() == TrackingId(5));
    REQUIRE(std::ranges::none_of(report, [](const Package* package) {
        return package->getStatus() == PackageStatus::Delivered;
    }));
}

TEST_CASE("LogisticsManager cannot be copied or moved") {
    STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<LogisticsManager>);
    STATIC_REQUIRE_FALSE(std::is_copy_assignable_v<LogisticsManager>);
    STATIC_REQUIRE_FALSE(std::is_move_constructible_v<LogisticsManager>);
    STATIC_REQUIRE_FALSE(std::is_move_assignable_v<LogisticsManager>);
}
