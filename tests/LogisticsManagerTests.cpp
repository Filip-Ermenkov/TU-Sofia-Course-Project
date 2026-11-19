#include <catch2/catch_test_macros.hpp>
#include <type_traits>

#include "courier/LogisticsManager.hpp"
#include "courier/PackageNotFoundException.hpp"
#include "courier/ValidationException.hpp"

using courier::Dimensions;
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

TEST_CASE("LogisticsManager cannot be copied or moved") {
    STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<LogisticsManager>);
    STATIC_REQUIRE_FALSE(std::is_copy_assignable_v<LogisticsManager>);
    STATIC_REQUIRE_FALSE(std::is_move_constructible_v<LogisticsManager>);
    STATIC_REQUIRE_FALSE(std::is_move_assignable_v<LogisticsManager>);
}
