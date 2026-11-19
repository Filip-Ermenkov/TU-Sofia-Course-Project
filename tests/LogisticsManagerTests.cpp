#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "courier/AssigneeNotFoundException.hpp"
#include "courier/Courier.hpp"
#include "courier/InvalidStatusChangeException.hpp"
#include "courier/LogisticsManager.hpp"
#include "courier/Office.hpp"
#include "courier/OperationNotAllowedException.hpp"
#include "courier/OperationType.hpp"
#include "courier/PackageNotFoundException.hpp"
#include "courier/ParcelDetails.hpp"
#include "courier/StatusObserver.hpp"
#include "courier/ValidationException.hpp"

using courier::AssigneeKind;
using courier::AssigneeNotFoundException;
using courier::Courier;
using courier::Dimensions;
using courier::InvalidStatusChangeException;
using courier::LogisticsManager;
using courier::Money;
using courier::Office;
using courier::OperationNotAllowedException;
using courier::OperationType;
using courier::Package;
using courier::PackageNotFoundException;
using courier::PackageStatus;
using courier::ParcelDetails;
using courier::ServiceType;
using courier::StatusObserver;
using courier::TrackingId;
using courier::ValidationException;

namespace {

const Dimensions kBox(10.0);

struct Notification {
    std::uint32_t id;
    PackageStatus previous;
    PackageStatus current;
};

// Remembers every notification, so the tests can check who was told what and in which order.
class RecordingObserver final : public StatusObserver {
public:
    explicit RecordingObserver(std::vector<std::string>* order = nullptr, std::string name = "")
        : order_(order), name_(std::move(name)) {}

    void onStatusChanged(const Package& package, PackageStatus previous) noexcept override {
        notifications_.push_back({package.getId().getNumber(), previous, package.getStatus()});
        if (order_ != nullptr) {
            order_->push_back(name_);
        }
    }

    [[nodiscard]] const std::vector<Notification>& notifications() const { return notifications_; }

private:
    std::vector<Notification> notifications_;
    std::vector<std::string>* order_;
    std::string name_;
};

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

TEST_CASE("StatusObserver is an abstract interface with a noexcept notification") {
    STATIC_REQUIRE(std::is_abstract_v<StatusObserver>);
    STATIC_REQUIRE(std::has_virtual_destructor_v<StatusObserver>);
    STATIC_REQUIRE(noexcept(std::declval<StatusObserver&>().onStatusChanged(
        std::declval<const Package&>(), PackageStatus::Accepted)));
}

TEST_CASE("LogisticsManager notifies observers after a status change") {
    LogisticsManager manager;
    RecordingObserver observer;
    manager.attach(observer);
    const Package& package = manager.accept(ServiceType::Standard, 1.0, kBox, "Ivan", "Sofia");
    REQUIRE(observer.notifications().empty());  // accepting is not a status change

    manager.changeStatus(package.getId(), PackageStatus::InTransit);
    manager.changeStatus(package.getId(), PackageStatus::Delivered);

    REQUIRE(observer.notifications().size() == 2);
    REQUIRE(observer.notifications()[0].id == 1);
    REQUIRE(observer.notifications()[0].previous == PackageStatus::Accepted);
    REQUIRE(observer.notifications()[0].current == PackageStatus::InTransit);
    REQUIRE(observer.notifications()[1].previous == PackageStatus::InTransit);
    REQUIRE(observer.notifications()[1].current == PackageStatus::Delivered);
}

TEST_CASE("LogisticsManager notifies nobody when a status change fails") {
    LogisticsManager manager;
    RecordingObserver observer;
    manager.attach(observer);
    const Package& package = manager.accept(ServiceType::Express, 1.0, kBox, "Ivan", "Sofia");

    REQUIRE_THROWS_AS(manager.changeStatus(package.getId(), PackageStatus::Delivered),
                      InvalidStatusChangeException);
    REQUIRE_THROWS_AS(manager.changeStatus(TrackingId(99), PackageStatus::InTransit),
                      PackageNotFoundException);
    REQUIRE(observer.notifications().empty());
}

TEST_CASE("LogisticsManager notifies observers when it dispatches a shipment") {
    LogisticsManager manager;
    RecordingObserver observer;
    manager.attach(observer);
    manager.accept(ServiceType::Standard, 1.0, kBox, "Ivan", "Sofia");
    manager.accept(ServiceType::Express, 1.0, kBox, "Maria", "Varna");

    REQUIRE(manager.dispatchNext() == TrackingId(2));
    REQUIRE(observer.notifications().size() == 1);
    REQUIRE(observer.notifications()[0].id == 2);
    REQUIRE(observer.notifications()[0].current == PackageStatus::InTransit);
}

TEST_CASE("LogisticsManager notifies every observer once, in the order they were attached") {
    LogisticsManager manager;
    std::vector<std::string> order;
    RecordingObserver first(&order, "first");
    RecordingObserver second(&order, "second");
    manager.attach(first);
    manager.attach(second);
    manager.attach(first);  // already attached: ignored
    REQUIRE(manager.observerCount() == 2);

    const Package& package = manager.accept(ServiceType::Fragile, 1.0, kBox, "Ivan", "Sofia");
    manager.changeStatus(package.getId(), PackageStatus::InTransit);

    REQUIRE(order == std::vector<std::string>{"first", "second"});
}

TEST_CASE("LogisticsManager stops notifying a detached observer") {
    LogisticsManager manager;
    RecordingObserver kept;
    RecordingObserver removed;
    manager.attach(kept);
    manager.attach(removed);
    manager.detach(removed);
    manager.detach(removed);  // not attached any more: nothing happens
    REQUIRE(manager.observerCount() == 1);

    const Package& package = manager.accept(ServiceType::Standard, 1.0, kBox, "Ivan", "Sofia");
    manager.changeStatus(package.getId(), PackageStatus::InTransit);

    REQUIRE(kept.notifications().size() == 1);
    REQUIRE(removed.notifications().empty());
}

TEST_CASE("LogisticsManager starts with an empty history") {
    const LogisticsManager manager;
    REQUIRE(manager.history().empty());
}

TEST_CASE("LogisticsManager records accepted shipments and status changes") {
    LogisticsManager manager;
    const auto& package =
        manager.accept(ServiceType::Express, 1.0, kBox, "Georgi Dimitrov", "Varna, 10 Sea Garden");
    REQUIRE(manager.dispatchNext() == package.getId());
    manager.changeStatus(package.getId(), PackageStatus::Delivered);

    const auto& history = manager.history();
    REQUIRE(history.size() == 3);
    REQUIRE(history[0].getType() == OperationType::Accept);
    REQUIRE(history[0].getId() == package.getId());
    REQUIRE(history[0].getDetails() ==
            "Express shipment to Varna, 10 Sea Garden for Georgi Dimitrov");
    REQUIRE(history[1].getType() == OperationType::StatusChange);
    REQUIRE(history[1].getDetails() == "Accepted -> In transit");
    REQUIRE(history[2].getType() == OperationType::StatusChange);
    REQUIRE(history[2].getDetails() == "In transit -> Delivered");
}

TEST_CASE("LogisticsManager records nothing for a failed operation") {
    LogisticsManager manager;
    REQUIRE_THROWS_AS(manager.accept(ServiceType::Standard, -1.0, kBox, "Ivan", "Sofia"),
                      ValidationException);
    REQUIRE(manager.history().empty());

    const auto& package = manager.accept(ServiceType::Standard, 1.0, kBox, "Ivan", "Sofia");
    REQUIRE_THROWS_AS(manager.changeStatus(package.getId(), PackageStatus::Delivered),
                      InvalidStatusChangeException);
    REQUIRE_THROWS_AS(manager.changeStatus(TrackingId(99), PackageStatus::InTransit),
                      PackageNotFoundException);
    REQUIRE(manager.history().size() == 1);
}

TEST_CASE("LogisticsManager reroutes a shipment with undo and redo") {
    LogisticsManager manager;
    const auto& package = manager.accept(ServiceType::Standard, 1.0, kBox, "Georgi", "Varna");
    const TrackingId id = package.getId();
    REQUIRE_FALSE(manager.canUndo());

    manager.reroute(id, "Burgas");
    REQUIRE(package.getDestination() == "Burgas");
    REQUIRE(manager.canUndo());

    REQUIRE(manager.undo());
    REQUIRE(package.getDestination() == "Varna");
    REQUIRE(manager.canRedo());

    REQUIRE(manager.redo());
    REQUIRE(package.getDestination() == "Burgas");

    const auto& history = manager.history();
    REQUIRE(history.size() == 4);
    REQUIRE(history[1].getType() == OperationType::Reroute);
    REQUIRE(history[1].getDetails() == "Reroute CR00000001: Varna -> Burgas");
    REQUIRE(history[2].getType() == OperationType::Undo);
    REQUIRE(history[2].getDetails() == "Reroute CR00000001: Varna -> Burgas");
    REQUIRE(history[3].getType() == OperationType::Redo);
}

TEST_CASE("LogisticsManager reroutes to a new recipient") {
    LogisticsManager manager;
    const auto& package = manager.accept(ServiceType::Standard, 1.0, kBox, "Georgi", "Varna");
    manager.reroute(package.getId(), "Burgas", "Anna");
    REQUIRE(package.getRecipient() == "Anna");
    REQUIRE(manager.undo());
    REQUIRE(package.getRecipient() == "Georgi");
}

TEST_CASE("LogisticsManager modifies a shipment and the price follows") {
    LogisticsManager manager;
    const auto& package = manager.accept(ServiceType::Standard, 1.0, kBox, "Georgi", "Varna");
    const Money oldTotal = package.totalCost();

    manager.modify(package.getId(), ParcelDetails(4.0, kBox, Money(5000)));
    REQUIRE(package.getWeightKg() == 4.0);
    REQUIRE(package.totalCost() > oldTotal);
    REQUIRE(manager.history().back().getType() == OperationType::Modify);

    REQUIRE(manager.undo());
    REQUIRE(package.totalCost() == oldTotal);
}

TEST_CASE("LogisticsManager keeps every undo as a cancelled operation") {
    LogisticsManager manager;
    const auto& package = manager.accept(ServiceType::Standard, 1.0, kBox, "Georgi", "Varna");
    manager.reroute(package.getId(), "Burgas");
    REQUIRE(manager.undo());
    manager.reroute(package.getId(), "Ruse");  // clears the redo stack
    REQUIRE_FALSE(manager.canRedo());
    REQUIRE(manager.undo());

    const auto cancelled = manager.cancelledOperations();
    REQUIRE(cancelled.size() == 2);
    REQUIRE(cancelled[0].getDetails() == "Reroute CR00000001: Varna -> Burgas");
    REQUIRE(cancelled[1].getDetails() == "Reroute CR00000001: Varna -> Ruse");
}

TEST_CASE("LogisticsManager has nothing to undo or redo at first") {
    LogisticsManager manager;
    REQUIRE_FALSE(manager.undo());
    REQUIRE_FALSE(manager.redo());
    REQUIRE(manager.history().empty());
    REQUIRE(manager.cancelledOperations().empty());
}

TEST_CASE("LogisticsManager rejects bad changes without recording them") {
    LogisticsManager manager;
    const auto& package = manager.accept(ServiceType::Standard, 1.0, kBox, "Georgi", "Varna");
    REQUIRE_THROWS_AS(manager.reroute(TrackingId(99), "Burgas"), PackageNotFoundException);
    REQUIRE_THROWS_AS(manager.reroute(package.getId(), " "), ValidationException);
    REQUIRE_THROWS_AS(manager.modify(TrackingId(99), ParcelDetails(1.0, kBox)),
                      PackageNotFoundException);
    REQUIRE(manager.history().size() == 1);
    REQUIRE_FALSE(manager.canUndo());
}

TEST_CASE("LogisticsManager cannot change or undo changes to a delivered shipment") {
    LogisticsManager manager;
    const auto& package = manager.accept(ServiceType::Standard, 1.0, kBox, "Georgi", "Varna");
    manager.reroute(package.getId(), "Burgas");
    manager.changeStatus(package.getId(), PackageStatus::InTransit);
    manager.changeStatus(package.getId(), PackageStatus::Delivered);
    const auto recorded = manager.history().size();

    REQUIRE_THROWS_AS(manager.reroute(package.getId(), "Ruse"), OperationNotAllowedException);
    REQUIRE_THROWS_AS(manager.undo(), OperationNotAllowedException);
    REQUIRE(package.getDestination() == "Burgas");
    REQUIRE(manager.history().size() == recorded);
    REQUIRE(manager.canUndo());  // still on the stack: nothing was lost
}

TEST_CASE("LogisticsManager does not notify observers about reroutes") {
    RecordingObserver observer;
    LogisticsManager manager;
    manager.attach(observer);
    const auto& package = manager.accept(ServiceType::Express, 1.0, kBox, "Georgi", "Varna");
    manager.reroute(package.getId(), "Burgas");
    REQUIRE(manager.undo());
    REQUIRE(observer.notifications().empty());
}

TEST_CASE("LogisticsManager registers couriers and offices") {
    LogisticsManager manager;
    REQUIRE(manager.assigneeCount() == 0);
    const auto& courier =
        manager.addAssignee(std::make_unique<Courier>("C-01", "Ivan Petrov", "0888 123 456"));
    manager.addAssignee(std::make_unique<Office>("VAR-1", "Varna Center"));

    REQUIRE(courier.getName() == "Ivan Petrov");
    REQUIRE(manager.assigneeCount() == 2);
    REQUIRE(manager.hasAssignee("C-01"));
    REQUIRE_FALSE(manager.hasAssignee("C-02"));
    REQUIRE(&manager.getAssignee("C-01") == &courier);
    REQUIRE(manager.getAssignee("VAR-1").kind() == AssigneeKind::Office);
    REQUIRE(manager.history().empty());  // not an operation on a shipment
}

TEST_CASE("LogisticsManager lists couriers and offices ordered by code") {
    LogisticsManager manager;
    manager.addAssignee(std::make_unique<Office>("VAR-1", "Varna Center"));
    manager.addAssignee(std::make_unique<Courier>("C-02", "Maria Ivanova"));
    manager.addAssignee(std::make_unique<Courier>("C-01", "Ivan Petrov"));

    std::vector<std::string> codes;
    for (const courier::Assignee* assignee : manager.assignees()) {
        codes.push_back(assignee->getCode());
    }
    REQUIRE(codes == std::vector<std::string>{"C-01", "C-02", "VAR-1"});
}

TEST_CASE("LogisticsManager rejects a duplicate code and keeps the first assignee") {
    LogisticsManager manager;
    manager.addAssignee(std::make_unique<Courier>("C-01", "Ivan Petrov"));
    REQUIRE_THROWS_AS(manager.addAssignee(std::make_unique<Office>("C-01", "Sofia Center")),
                      ValidationException);
    REQUIRE(manager.assigneeCount() == 1);
    REQUIRE(manager.getAssignee("C-01").getName() == "Ivan Petrov");
}

TEST_CASE("LogisticsManager rejects a missing assignee") {
    LogisticsManager manager;
    REQUIRE_THROWS_AS(manager.addAssignee(nullptr), std::invalid_argument);
    REQUIRE(manager.assigneeCount() == 0);
}

TEST_CASE("LogisticsManager reports an unknown courier or office code") {
    const LogisticsManager manager;
    REQUIRE_THROWS_AS(manager.getAssignee("C-99"), AssigneeNotFoundException);
    REQUIRE_THROWS_WITH(manager.getAssignee("C-99"), "No courier or office with code C-99");
}
