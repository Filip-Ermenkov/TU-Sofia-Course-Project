#include <catch2/catch_test_macros.hpp>
#include <sstream>
#include <string>
#include <vector>

#include "courier/DispatchModule.hpp"
#include "courier/LogisticsManager.hpp"

using courier::Dimensions;
using courier::DispatchModule;
using courier::LogisticsManager;
using courier::PackageStatus;
using courier::ServiceType;

namespace {

const Dimensions kBox(10.0);

}  // namespace

TEST_CASE("DispatchModule starts with no alerts") {
    const DispatchModule dispatch;
    REQUIRE(dispatch.alerts().empty());
}

TEST_CASE("DispatchModule records status changes of express and fragile shipments") {
    LogisticsManager manager;
    DispatchModule dispatch;
    manager.attach(dispatch);
    const auto& express = manager.accept(ServiceType::Express, 1.0, kBox, "Ivan", "Sofia");
    const auto& fragile = manager.accept(ServiceType::Fragile, 1.0, kBox, "Maria", "Varna");

    manager.changeStatus(express.getId(), PackageStatus::InTransit);
    manager.changeStatus(fragile.getId(), PackageStatus::InTransit);
    manager.changeStatus(express.getId(), PackageStatus::Delivered);

    REQUIRE(dispatch.alerts() == std::vector<std::string>{
                                     "CR00000001 (Express): Accepted -> In transit",
                                     "CR00000002 (Fragile): Accepted -> In transit",
                                     "CR00000001 (Express): In transit -> Delivered",
                                 });
}

TEST_CASE("DispatchModule ignores standard shipments") {
    LogisticsManager manager;
    DispatchModule dispatch;
    manager.attach(dispatch);
    const auto& standard = manager.accept(ServiceType::Standard, 1.0, kBox, "Ivan", "Sofia");

    manager.changeStatus(standard.getId(), PackageStatus::InTransit);
    REQUIRE(dispatch.alerts().empty());
}

TEST_CASE("DispatchModule prints each alert to the stream it was given") {
    std::ostringstream out;
    LogisticsManager manager;
    DispatchModule dispatch(out);
    manager.attach(dispatch);
    manager.accept(ServiceType::Standard, 1.0, kBox, "Ivan", "Sofia");
    manager.accept(ServiceType::Fragile, 1.0, kBox, "Maria", "Varna");

    manager.dispatchNext();  // Standard first (no express waiting): not printed
    manager.dispatchNext();

    REQUIRE(out.str() == "[dispatch] CR00000002 (Fragile): Accepted -> In transit\n");
    REQUIRE(dispatch.alerts().size() == 1);
}
