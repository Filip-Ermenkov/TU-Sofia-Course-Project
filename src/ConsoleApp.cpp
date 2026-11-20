#include "courier/ConsoleApp.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdlib>
#include <format>
#include <functional>
#include <iostream>
#include <string>
#include <utility>

#include "courier/AssigneeNotFoundException.hpp"
#include "courier/ConsoleFormat.hpp"
#include "courier/Dimensions.hpp"
#include "courier/InputParsing.hpp"
#include "courier/InvalidStatusChangeException.hpp"
#include "courier/LogisticsException.hpp"
#include "courier/Money.hpp"
#include "courier/OperationNotAllowedException.hpp"
#include "courier/PackageNotFoundException.hpp"
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

}  // namespace courier
