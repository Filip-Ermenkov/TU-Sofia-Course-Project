#include "courier/ConsoleApp.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdlib>
#include <format>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "courier/Assignee.hpp"
#include "courier/AssigneeNotFoundException.hpp"
#include "courier/ConsoleFormat.hpp"
#include "courier/Courier.hpp"
#include "courier/Dimensions.hpp"
#include "courier/InputParsing.hpp"
#include "courier/InvalidStatusChangeException.hpp"
#include "courier/LogisticsException.hpp"
#include "courier/Money.hpp"
#include "courier/Office.hpp"
#include "courier/OperationNotAllowedException.hpp"
#include "courier/PackageNotFoundException.hpp"
#include "courier/ParcelDetails.hpp"
#include "courier/StorageException.hpp"
#include "courier/ValidationException.hpp"

namespace courier {

namespace {

constexpr std::array kServiceTypes{ServiceType::Standard, ServiceType::Express,
                                   ServiceType::Fragile};
constexpr std::array kStatuses{PackageStatus::Accepted, PackageStatus::InTransit,
                               PackageStatus::Delivered};

// The display names of the values, in the same order, for InputReader::askChoice.
template <typename Enum, std::size_t N>
std::vector<std::string_view> namesOf(const std::array<Enum, N>& values) {
    std::vector<std::string_view> names;
    names.reserve(N);
    for (const Enum value : values) {
        names.push_back(toString(value));
    }
    return names;
}

// Exact comparison is right here: a value the user kept by pressing Enter is the same double.
bool sameDetails(const Package& package, const ParcelDetails& details) {
    const Dimensions& a = package.getDimensions();
    const Dimensions& b = details.getDimensions();
    return package.getWeightKg() == details.getWeightKg() && a.getLength() == b.getLength() &&
           a.getWidth() == b.getWidth() && a.getHeight() == b.getHeight() &&
           package.getDeclaredValue() == details.getDeclaredValue();
}

}  // namespace

ConsoleApp::ConsoleApp() : ConsoleApp(std::cin, std::cout, CsvStorage()) {}

ConsoleApp::ConsoleApp(std::istream& in, std::ostream& out, CsvStorage storage)
    : out_(out),
      input_(in, out),
      storage_(std::move(storage)),
      zone_(localTimeZone()),
      dispatch_(out),
      menu_(buildMenu()) {}

std::vector<ConsoleApp::MenuItem> ConsoleApp::buildMenu() {
    return {
        {1, "Shipments", "Accept a new shipment", [this] { return acceptShipment(); }},
        {2, "Shipments", "Find a shipment by tracking ID", [this] { return findShipment(); }},
        {3, "Shipments", "List all shipments", [this] { return listShipments(); }},
        {4, "Shipments", "Report: undelivered shipments", [this] { return reportUndelivered(); }},
        {5, "Shipments", "Filter shipments", [this] { return filterShipments(); }},
        {6, "Delivery", "Dispatch the next waiting shipment", [this] { return dispatchNext(); }},
        {7, "Delivery", "Change the status of a shipment", [this] { return changeStatus(); }},
        {8, "Delivery", "Show the dispatch alerts of this session",
         [this] { return showDispatchAlerts(); }},
        {9, "Changes (can be undone)", "Reroute a shipment", [this] { return rerouteShipment(); }},
        {10, "Changes (can be undone)", "Modify a shipment's weight, size or declared value",
         [this] { return modifyShipment(); }},
        {11, "Changes (can be undone)", "Assign a shipment to a courier or office",
         [this] { return assignShipment(); }},
        {12, "Changes (can be undone)", "Remove a shipment's assignment",
         [this] { return unassignShipment(); }},
        {13, "Changes (can be undone)", "Undo the last change", [this] { return undo(); }},
        {14, "Changes (can be undone)", "Redo the last undone change", [this] { return redo(); }},
        {15, "Couriers and offices", "Register a courier", [this] { return registerCourier(); }},
        {16, "Couriers and offices", "Register an office", [this] { return registerOffice(); }},
        {17, "Couriers and offices", "List couriers and offices with their shipments",
         [this] { return listAssignees(); }},
    };
}

int ConsoleApp::run() {
    out_ << "Courier Shipment Management System\n";
    if (!loadData()) {
        return EXIT_FAILURE;
    }
    // The loaded manager is new, so the dispatch module is attached only now.
    manager_->attach(dispatch_);

    while (true) {
        showMenu();
        const auto line = input_.readLine("Choose: ");
        if (!line) {
            break;
        }
        const auto choice = parseInteger(*line);
        if (choice == 0) {
            if (confirmExit()) {
                return finish();
            }
            continue;
        }
        const auto item = std::ranges::find_if(
            menu_, [&choice](const MenuItem& entry) { return entry.key == choice; });
        if (item == menu_.end()) {
            out_ << "Please choose a number from the menu.\n";
            continue;
        }
        runAction(*item);
        if (input_.atEnd()) {
            break;
        }
    }
    // The input has ended, so nobody can be asked: one last try to save.
    if (unsaved_) {
        saveData();
    }
    return finish();
}

int ConsoleApp::finish() const {
    out_ << "Goodbye.\n";
    return unsaved_ ? EXIT_FAILURE : EXIT_SUCCESS;
}

bool ConsoleApp::loadData() {
    const std::string folder = storage_.getDirectory().string();
    try {
        manager_ = storage_.load();
    } catch (const StorageException& error) {
        out_ << "Could not load the saved data: " << error.what() << '\n';
        if (!input_.askYesNo(std::format("Start with no data? The files in {} will be replaced "
                                         "when the first change is saved.",
                                         folder))) {
            out_ << "The files were left as they are.\n";
            return false;
        }
        manager_ = std::make_unique<LogisticsManager>();
        return true;
    }
    out_ << std::format(
        "Loaded {} shipments, {} couriers and offices and {} history entries from {}.\n",
        manager_->packageCount(), manager_->assigneeCount(), manager_->history().size(), folder);
    return true;
}

void ConsoleApp::saveData() {
    try {
        storage_.save(*manager_);
        unsaved_ = false;
    } catch (const StorageException& error) {
        unsaved_ = true;
        out_ << "Warning: the changes could not be saved: " << error.what() << '\n'
             << "They are kept in memory; saving is tried again after the next change and on "
                "exit.\n";
    }
}

void ConsoleApp::showMenu() const {
    out_ << std::format("\n{} shipments ({} waiting), {} couriers and offices\n",
                        manager_->packageCount(), manager_->waitingCount(),
                        manager_->assigneeCount());
    std::string_view section;
    for (const MenuItem& item : menu_) {
        if (item.section != section) {
            section = item.section;
            out_ << section << '\n';
        }
        out_ << std::format("  {:>2}  {}\n", item.key, item.label);
    }
    out_ << std::format("  {:>2}  {}\n", 0, "Exit");
}

void ConsoleApp::runAction(const MenuItem& item) {
    bool changed = false;
    // Most specific first: a handler for a base class would also catch its subclasses.
    try {
        changed = item.action();
    } catch (const ValidationException& error) {
        out_ << "Rejected: " << error.what() << '\n';
    } catch (const PackageNotFoundException& error) {
        out_ << "Not found: " << error.what() << '\n';
    } catch (const AssigneeNotFoundException& error) {
        out_ << "Not found: " << error.what() << '\n';
    } catch (const InvalidStatusChangeException& error) {
        out_ << "Not allowed: " << error.what() << '\n';
    } catch (const OperationNotAllowedException& error) {
        out_ << "Not allowed: " << error.what() << '\n';
    } catch (const LogisticsException& error) {
        out_ << "Error: " << error.what() << '\n';
    }
    if (changed) {
        saveData();
    }
}

bool ConsoleApp::confirmExit() {
    if (!unsaved_) {
        return true;
    }
    saveData();
    return !unsaved_ || input_.askYesNo("Exit anyway and lose the unsaved changes?");
}

void ConsoleApp::printPackages(const std::vector<const Package*>& packages) const {
    if (packages.empty()) {
        out_ << "  (none)\n";
        return;
    }
    out_ << summaryHeader() << '\n';
    for (const Package* package : packages) {
        out_ << formatSummary(*package) << '\n';
    }
}

std::optional<ServiceType> ConsoleApp::askServiceType() {
    const auto choice = input_.askChoice("Service", namesOf(kServiceTypes));
    if (!choice) {
        return std::nullopt;
    }
    return kServiceTypes.at(*choice);
}

std::optional<PackageStatus> ConsoleApp::askStatus(std::string_view prompt) {
    const auto choice = input_.askChoice(prompt, namesOf(kStatuses));
    if (!choice) {
        return std::nullopt;
    }
    return kStatuses.at(*choice);
}

bool ConsoleApp::acceptShipment() {
    const auto type = askServiceType();
    if (!type) {
        return false;
    }
    const auto weight =
        input_.askNumber(std::format("Weight in kg (up to {})", Package::kMaxWeightKg));
    if (!weight) {
        return false;
    }
    const std::string sidePrompt = std::format("in cm (up to {})", Dimensions::kMaxSideCm);
    const auto length = input_.askNumber("Length " + sidePrompt);
    if (!length) {
        return false;
    }
    const auto width = input_.askNumber("Width " + sidePrompt);
    if (!width) {
        return false;
    }
    const auto height = input_.askNumber("Height " + sidePrompt);
    if (!height) {
        return false;
    }
    auto recipient = input_.askText("Recipient");
    if (!recipient) {
        return false;
    }
    auto destination = input_.askText("Destination");
    if (!destination) {
        return false;
    }
    const auto declaredValue = input_.askMoney(
        std::format("Declared value (up to {})", Package::kMaxDeclaredValue.toString()), Money());
    if (!declaredValue) {
        return false;
    }
    const Package& package =
        manager_->accept(*type, *weight, Dimensions(*length, *width, *height),
                         std::move(*recipient), std::move(*destination), *declaredValue);
    out_ << "Shipment accepted.\n" << package;
    return true;
}

bool ConsoleApp::findShipment() {
    const auto id = input_.askTrackingId("Tracking ID");
    if (!id) {
        return false;
    }
    out_ << manager_->get(*id);
    return false;
}

bool ConsoleApp::listShipments() {
    const auto order =
        input_.askChoice("Order", {"By tracking ID", "By total cost, highest first"});
    if (!order) {
        return false;
    }
    auto packages = manager_->filter([](const Package&) { return true; });
    if (*order == 1) {
        // Stable, so shipments that cost the same stay in tracking ID order.
        std::ranges::stable_sort(packages, std::ranges::greater{},
                                 [](const Package* package) { return package->totalCost(); });
    }
    printPackages(packages);
    return false;
}

bool ConsoleApp::reportUndelivered() {
    out_ << "Waiting shipments first, then those in transit:\n";
    printPackages(manager_->undelivered());
    return false;
}

bool ConsoleApp::filterShipments() {
    const auto kind = input_.askChoice(
        "Filter",
        {"By service", "By status", "By destination (contains text)", "By total cost (at least)"});
    if (!kind) {
        return false;
    }
    LogisticsManager::PackagePredicate predicate;
    switch (*kind) {
        case 0:
            if (const auto type = askServiceType()) {
                predicate = [type = *type](const Package& package) {
                    return package.serviceType() == type;
                };
            }
            break;
        case 1:
            if (const auto status = askStatus("Status")) {
                predicate = [status = *status](const Package& package) {
                    return package.getStatus() == status;
                };
            }
            break;
        case 2:
            if (const auto text = input_.askText("Destination contains")) {
                // Upper-cased on both sides, so the letter case does not matter.
                predicate = [part = toUpper(*text)](const Package& package) {
                    return toUpper(package.getDestination()).find(part) != std::string::npos;
                };
            }
            break;
        case 3:
            if (const auto minimum = input_.askMoney("Total cost at least")) {
                predicate = [minimum = *minimum](const Package& package) {
                    return package.totalCost() >= minimum;
                };
            }
            break;
    }
    if (predicate) {
        printPackages(manager_->filter(predicate));
    }
    return false;
}

bool ConsoleApp::dispatchNext() {
    // Express shipments go first, then the others in the order they arrived.
    const auto id = manager_->dispatchNext();
    if (!id) {
        out_ << "No shipment is waiting.\n";
        return false;
    }
    out_ << std::format("{} is now in transit.\n", id->getValue());
    return true;
}

bool ConsoleApp::changeStatus() {
    const auto id = input_.askTrackingId("Tracking ID");
    if (!id) {
        return false;
    }
    out_ << std::format("Current status: {}\n", toString(manager_->get(*id).getStatus()));
    const auto status = askStatus("New status");
    if (!status) {
        return false;
    }
    manager_->changeStatus(*id, *status);
    out_ << std::format("{} is now {}.\n", id->getValue(), toString(*status));
    return true;
}

bool ConsoleApp::showDispatchAlerts() {
    // The dispatch module hears about every status change but keeps only express and fragile ones.
    if (dispatch_.alerts().empty()) {
        out_ << "  (none)\n";
    }
    for (const std::string& alert : dispatch_.alerts()) {
        out_ << "  " << alert << '\n';
    }
    return false;
}

bool ConsoleApp::rerouteShipment() {
    const auto id = input_.askTrackingId("Tracking ID");
    if (!id) {
        return false;
    }
    const Package& package = manager_->get(*id);
    out_ << "Press Enter to keep a value as it is.\n";
    auto destination = input_.askText("Destination", package.getDestination());
    if (!destination) {
        return false;
    }
    auto recipient = input_.askText("Recipient", package.getRecipient());
    if (!recipient) {
        return false;
    }
    const bool newRecipient = *recipient != package.getRecipient();
    if (*destination == package.getDestination() && !newRecipient) {
        out_ << "Nothing changed.\n";
        return false;
    }
    if (newRecipient) {
        manager_->reroute(*id, std::move(*destination), std::move(*recipient));
    } else {
        manager_->reroute(*id, std::move(*destination));
    }
    out_ << "Done: " << manager_->history().back().getDetails() << '\n';
    return true;
}

bool ConsoleApp::modifyShipment() {
    const auto id = input_.askTrackingId("Tracking ID");
    if (!id) {
        return false;
    }
    const Package& package = manager_->get(*id);
    const Dimensions& size = package.getDimensions();
    out_ << "Press Enter to keep a value as it is.\n";
    const auto weight = input_.askNumber(
        std::format("Weight in kg (up to {})", Package::kMaxWeightKg), package.getWeightKg());
    if (!weight) {
        return false;
    }
    const std::string sidePrompt = std::format("in cm (up to {})", Dimensions::kMaxSideCm);
    const auto length = input_.askNumber("Length " + sidePrompt, size.getLength());
    if (!length) {
        return false;
    }
    const auto width = input_.askNumber("Width " + sidePrompt, size.getWidth());
    if (!width) {
        return false;
    }
    const auto height = input_.askNumber("Height " + sidePrompt, size.getHeight());
    if (!height) {
        return false;
    }
    const auto declaredValue = input_.askMoney(
        std::format("Declared value (up to {})", Package::kMaxDeclaredValue.toString()),
        package.getDeclaredValue());
    if (!declaredValue) {
        return false;
    }
    // Checks all the new values before anything is changed.
    const ParcelDetails details(*weight, Dimensions(*length, *width, *height), *declaredValue);
    if (sameDetails(package, details)) {
        out_ << "Nothing changed.\n";
        return false;
    }
    manager_->modify(*id, details);
    out_ << "Done: " << manager_->history().back().getDetails() << '\n'
         << std::format("New total: {}\n", package.totalCost().toString());
    return true;
}

bool ConsoleApp::undo() {
    if (!manager_->undo()) {
        out_ << "Nothing to undo.\n";
        return false;
    }
    out_ << "Undone: " << manager_->history().back().getDetails() << '\n';
    return true;
}

bool ConsoleApp::redo() {
    if (!manager_->redo()) {
        out_ << "Nothing to redo.\n";
        return false;
    }
    out_ << "Redone: " << manager_->history().back().getDetails() << '\n';
    return true;
}

bool ConsoleApp::assignShipment() {
    const auto id = input_.askTrackingId("Tracking ID");
    if (!id) {
        return false;
    }
    const Package& package = manager_->get(*id);
    out_ << std::format("Assigned to: {}\n", package.getAssigneeCode().value_or("-"));
    const auto code = input_.askCode("Courier or office code");
    if (!code) {
        return false;
    }
    if (package.getAssigneeCode() == code) {
        out_ << "Nothing changed.\n";
        return false;
    }
    manager_->assign(*id, *code);
    out_ << "Done: " << manager_->history().back().getDetails() << '\n';
    return true;
}

bool ConsoleApp::unassignShipment() {
    const auto id = input_.askTrackingId("Tracking ID");
    if (!id) {
        return false;
    }
    if (!manager_->get(*id).getAssigneeCode()) {
        out_ << std::format("{} is not assigned to anyone.\n", id->getValue());
        return false;
    }
    manager_->unassign(*id);
    out_ << "Done: " << manager_->history().back().getDetails() << '\n';
    return true;
}

std::optional<ConsoleApp::AssigneeInput> ConsoleApp::askAssignee(std::string_view contactPrompt) {
    auto code = input_.askCode(
        std::format("Code (1 to {} characters of A-Z, 0-9 and '-')", Assignee::kMaxCodeLength));
    if (!code) {
        return std::nullopt;
    }
    auto name = input_.askText("Name");
    if (!name) {
        return std::nullopt;
    }
    auto contact = input_.askText(contactPrompt, "");
    if (!contact) {
        return std::nullopt;
    }
    return AssigneeInput{std::move(*code), std::move(*name), std::move(*contact)};
}

bool ConsoleApp::registerCourier() {
    auto input = askAssignee("Phone");
    if (!input) {
        return false;
    }
    // The phone is optional: without one, the two-argument constructor.
    auto courier = input->contact.empty()
                       ? std::make_unique<Courier>(std::move(input->code), std::move(input->name))
                       : std::make_unique<Courier>(std::move(input->code), std::move(input->name),
                                                   std::move(input->contact));
    out_ << "Registered: " << manager_->addAssignee(std::move(courier)) << '\n';
    return true;
}

bool ConsoleApp::registerOffice() {
    auto input = askAssignee("Address");
    if (!input) {
        return false;
    }
    auto office = input->contact.empty()
                      ? std::make_unique<Office>(std::move(input->code), std::move(input->name))
                      : std::make_unique<Office>(std::move(input->code), std::move(input->name),
                                                 std::move(input->contact));
    out_ << "Registered: " << manager_->addAssignee(std::move(office)) << '\n';
    return true;
}

bool ConsoleApp::listAssignees() {
    const auto assignees = manager_->assignees();
    if (assignees.empty()) {
        out_ << "  (none)\n";
    }
    for (const Assignee* assignee : assignees) {
        out_ << '\n' << *assignee << '\n';
        printPackages(manager_->assignedTo(assignee->getCode()));
    }
    return false;
}

}  // namespace courier
