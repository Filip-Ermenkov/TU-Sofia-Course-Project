#include <catch2/catch_test_macros.hpp>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>

#include "courier/ConsoleApp.hpp"
#include "courier/CsvStorage.hpp"
#include "courier/Dimensions.hpp"
#include "courier/LogisticsManager.hpp"
#include "courier/PackageStatus.hpp"
#include "courier/ServiceType.hpp"
#include "courier/TrackingId.hpp"

using courier::ConsoleApp;
using courier::CsvStorage;
namespace fs = std::filesystem;

namespace {

// A fresh folder under the system's temporary directory, deleted with everything in it at the end.
class TempDir {
public:
    TempDir()
        : path_(fs::temp_directory_path() /
                ("courier-app-tests-" + std::to_string(std::random_device{}()))) {
        fs::create_directories(path_);
    }
    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;
    TempDir(TempDir&&) = delete;
    TempDir& operator=(TempDir&&) = delete;
    ~TempDir() {
        std::error_code ignored;
        fs::remove_all(path_, ignored);
    }

    [[nodiscard]] const fs::path& path() const { return path_; }

private:
    fs::path path_;
};

// What a whole session showed, and how it ended.
struct Result {
    int exitCode;
    std::string shown;
};

// Runs the program on data in `folder`, with `typed` as everything the user types.
Result runSession(const fs::path& folder, const std::string& typed) {
    std::istringstream in(typed);
    std::ostringstream out;
    ConsoleApp app(in, out, CsvStorage(folder));
    const int exitCode = app.run();
    return {exitCode, out.str()};
}

bool contains(const std::string& text, const std::string& part) {
    return text.find(part) != std::string::npos;
}

// Saves two shipments, so a session has something to work with.
void saveTwoShipments(const fs::path& folder) {
    courier::LogisticsManager manager;
    const courier::Dimensions box(30.0, 20.0, 10.0);
    manager.accept(courier::ServiceType::Standard, 2.0, box, "Ivan Petrov", "Sofia");
    manager.accept(courier::ServiceType::Express, 3.0, box, "Maria Ivanova", "Varna");
    CsvStorage(folder).save(manager);
}

std::string readFile(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream text;
    text << in.rdbuf();
    return text.str();
}

}  // namespace

TEST_CASE("ConsoleApp starts empty on the first run and exits on 0") {
    const TempDir dir;
    const Result result = runSession(dir.path() / "data", "0\n");
    REQUIRE(result.exitCode == EXIT_SUCCESS);
    REQUIRE(contains(result.shown, "Loaded 0 shipments, 0 couriers and offices and 0 history"));
    REQUIRE(contains(result.shown, "0 shipments (0 waiting), 0 couriers and offices\n"));
    REQUIRE(
        contains(result.shown,
                 "Shipments\n   1  Accept a new shipment\n   2  Find a shipment by tracking ID\n"));
    REQUIRE(contains(result.shown,
                     "History\n  18  Logistics history\n  19  Cancelled (undone) "
                     "operations\n   0  Exit\n"));
    REQUIRE(result.shown.ends_with("Goodbye.\n"));
    // Nothing changed, so nothing was written.
    REQUIRE_FALSE(fs::exists(dir.path() / "data"));
}

TEST_CASE("ConsoleApp ends when the input ends") {
    const TempDir dir;
    const Result result = runSession(dir.path(), "");
    REQUIRE(result.exitCode == EXIT_SUCCESS);
    REQUIRE(result.shown.ends_with("Choose: \nGoodbye.\n"));
}

TEST_CASE("ConsoleApp asks again for a choice that is not on the menu") {
    const TempDir dir;
    const Result result = runSession(dir.path(), "abc\n42\n0\n");
    REQUIRE(result.exitCode == EXIT_SUCCESS);
    REQUIRE(contains(result.shown, "Choose: Please choose a number from the menu.\n"));
}

TEST_CASE("ConsoleApp loads the saved data and lists it") {
    const TempDir dir;
    saveTwoShipments(dir.path());
    const Result result = runSession(dir.path(), "3\n1\n0\n");
    REQUIRE(contains(result.shown, "Loaded 2 shipments, 0 couriers and offices and 2 history"));
    REQUIRE(contains(result.shown, "2 shipments (2 waiting)"));
    REQUIRE(contains(result.shown, "CR00000001  Standard  Accepted    -           Sofia"));
    REQUIRE(contains(result.shown, "CR00000002  Express   Accepted    -           Varna"));
}

TEST_CASE("ConsoleApp lists no shipments as none") {
    const TempDir dir;
    REQUIRE(contains(runSession(dir.path(), "3\n1\n0\n").shown, "  (none)\n"));
}

TEST_CASE("ConsoleApp finds a shipment and prints its waybill") {
    const TempDir dir;
    saveTwoShipments(dir.path());
    const Result result = runSession(dir.path(), "2\ncr00000002\n0\n");
    REQUIRE(contains(result.shown, "===== WAYBILL CR00000002 =====\n"));
    REQUIRE(contains(result.shown, "Recipient:      Maria Ivanova\n"));
}

TEST_CASE("ConsoleApp reports an unknown tracking ID and carries on") {
    const TempDir dir;
    const Result result = runSession(dir.path(), "2\nCR00000099\n2\n\n0\n");
    REQUIRE(result.exitCode == EXIT_SUCCESS);
    REQUIRE(contains(result.shown, "Not found: No shipment with tracking ID CR00000099\n"));
    REQUIRE(result.shown.ends_with("Goodbye.\n"));
}

TEST_CASE("ConsoleApp leaves damaged files alone unless the user agrees to start empty") {
    const TempDir dir;
    const fs::path file = dir.path() / CsvStorage::kAssigneesFile;
    std::ofstream(file, std::ios::binary) << "not a header\n";

    const Result refused = runSession(dir.path(), "n\n");
    REQUIRE(refused.exitCode == EXIT_FAILURE);
    REQUIRE(contains(refused.shown, "Could not load the saved data: "));
    REQUIRE(contains(refused.shown, "Start with no data? The files in "));
    REQUIRE(contains(refused.shown, "The files were left as they are.\n"));
    REQUIRE(readFile(file) == "not a header\n");

    const Result accepted = runSession(dir.path(), "y\n3\n1\n0\n");
    REQUIRE(accepted.exitCode == EXIT_SUCCESS);
    REQUIRE(contains(accepted.shown, "0 shipments (0 waiting)"));
    REQUIRE(readFile(file) == "not a header\n");  // replaced only when a change is saved
}

namespace {

// Typed answers for accepting a 2 kg Standard shipment of 30 x 20 x 10 cm, not insured.
const std::string kAcceptStandard = "1\n1\n2\n30\n20\n10\nIvan Petrov\nSofia, 1 Vitosha Blvd\n\n";

}  // namespace

TEST_CASE("ConsoleApp accepts a shipment, shows its waybill and saves it") {
    const TempDir dir;
    const Result result = runSession(dir.path(), kAcceptStandard + "0\n");
    REQUIRE(result.exitCode == EXIT_SUCCESS);
    REQUIRE(contains(result.shown, "Weight in kg (up to 1000): "));
    REQUIRE(contains(result.shown, "Length in cm (up to 500): "));
    REQUIRE(contains(result.shown, "Declared value (up to 1000000.00) [0.00]: "));
    REQUIRE(contains(result.shown, "Shipment accepted.\n===== WAYBILL CR00000001 =====\n"));
    REQUIRE(contains(result.shown, "1 shipments (1 waiting)"));

    const auto saved = CsvStorage(dir.path()).load();
    REQUIRE(saved->packageCount() == 1);
    REQUIRE(saved->get(courier::TrackingId(1)).getRecipient() == "Ivan Petrov");
}

TEST_CASE("ConsoleApp continues the tracking numbers after a restart") {
    const TempDir dir;
    runSession(dir.path(), kAcceptStandard + "0\n");
    const Result second = runSession(dir.path(), kAcceptStandard + "0\n");
    REQUIRE(contains(second.shown, "===== WAYBILL CR00000002 =====\n"));
}

TEST_CASE("ConsoleApp rejects invalid values and saves nothing") {
    const TempDir dir;
    const Result result =
        runSession(dir.path(), "1\n1\n2000\n30\n20\n10\nIvan Petrov\nSofia\n\n0\n");
    REQUIRE(result.exitCode == EXIT_SUCCESS);
    REQUIRE(contains(result.shown, "Rejected: Invalid weight: 2000\n"));
    REQUIRE(contains(result.shown, "0 shipments (0 waiting)"));
    REQUIRE_FALSE(fs::exists(dir.path() / CsvStorage::kShipmentsFile));
}

TEST_CASE("ConsoleApp cancels an action on an empty line") {
    const TempDir dir;
    const Result result = runSession(dir.path(), "1\n1\n2\n\n0\n");
    REQUIRE(result.exitCode == EXIT_SUCCESS);
    REQUIRE(contains(result.shown, "0 shipments (0 waiting)"));
    REQUIRE_FALSE(contains(result.shown, "Rejected"));
    REQUIRE_FALSE(fs::exists(dir.path() / CsvStorage::kShipmentsFile));
}

TEST_CASE("ConsoleApp lists shipments by total cost") {
    const TempDir dir;
    saveTwoShipments(dir.path());  // Standard 8.00, Express 15.50
    const std::string shown = runSession(dir.path(), "3\n2\n0\n").shown;
    REQUIRE(shown.find("CR00000002  Express") < shown.find("CR00000001  Standard"));
}

TEST_CASE("ConsoleApp reports the undelivered shipments") {
    const TempDir dir;
    saveTwoShipments(dir.path());
    const std::string shown = runSession(dir.path(), "4\n0\n").shown;
    REQUIRE(contains(shown, "Waiting shipments first, then those in transit:\n"));
    REQUIRE(contains(shown, "CR00000001  Standard"));
    REQUIRE(contains(shown, "CR00000002  Express"));
}

TEST_CASE("ConsoleApp filters shipments with each kind of filter") {
    const TempDir dir;
    saveTwoShipments(dir.path());
    const auto filtered = [&dir](const std::string& typed) {
        const std::string shown = runSession(dir.path(), "5\n" + typed + "0\n").shown;
        return std::pair(contains(shown, "CR00000001  "), contains(shown, "CR00000002  "));
    };
    REQUIRE(filtered("1\n2\n") == std::pair(false, true));    // Express
    REQUIRE(filtered("2\n1\n") == std::pair(true, true));     // Accepted
    REQUIRE(filtered("2\n3\n") == std::pair(false, false));   // Delivered
    REQUIRE(filtered("3\nvar\n") == std::pair(false, true));  // destination, any case
    REQUIRE(filtered("4\n10\n") == std::pair(false, true));   // at least 10.00
    REQUIRE(filtered("4\n8\n") == std::pair(true, true));     // at least 8.00
}

TEST_CASE("ConsoleApp warns when saving fails and keeps the change in memory") {
    const TempDir dir;
    const fs::path taken = dir.path() / "taken";
    std::ofstream(taken) << "a file, not a folder";
    const Result result = runSession(taken, kAcceptStandard + "3\n1\n");
    REQUIRE(contains(result.shown, "Warning: the changes could not be saved: "));
    REQUIRE(contains(result.shown, "1 shipments (1 waiting)"));
    // The input ended with the change still unsaved.
    REQUIRE(result.exitCode == EXIT_FAILURE);
}

TEST_CASE("ConsoleApp asks before exiting with unsaved changes") {
    const TempDir dir;
    const fs::path taken = dir.path() / "taken";
    std::ofstream(taken) << "a file, not a folder";
    const Result result = runSession(taken, kAcceptStandard + "0\nn\n0\ny\n");
    REQUIRE(contains(result.shown, "Exit anyway and lose the unsaved changes? (y/N): "));
    REQUIRE(result.exitCode == EXIT_FAILURE);
    REQUIRE(result.shown.ends_with("Goodbye.\n"));
}

TEST_CASE("ConsoleApp dispatches express shipments first") {
    const TempDir dir;
    saveTwoShipments(dir.path());
    const Result result = runSession(dir.path(), "6\n6\n6\n0\n");
    const std::string& shown = result.shown;
    REQUIRE(shown.find("CR00000002 is now in transit.") <
            shown.find("CR00000001 is now in transit."));
    REQUIRE(contains(shown, "No shipment is waiting.\n"));
    // The dispatch module is attached and reports the express shipment only.
    REQUIRE(contains(shown, "[dispatch] CR00000002 (Express): Accepted -> In transit\n"));
    REQUIRE_FALSE(contains(shown, "[dispatch] CR00000001"));
    REQUIRE(CsvStorage(dir.path()).load()->waitingCount() == 0);
}

TEST_CASE("ConsoleApp changes a status one step forward") {
    const TempDir dir;
    saveTwoShipments(dir.path());
    const Result result = runSession(dir.path(), "7\nCR00000002\n2\n7\nCR00000002\n3\n8\n0\n");
    REQUIRE(contains(result.shown, "Current status: Accepted\n"));
    REQUIRE(contains(result.shown, "CR00000002 is now Delivered.\n"));
    REQUIRE(contains(result.shown, "  CR00000002 (Express): In transit -> Delivered\n"));
    REQUIRE(CsvStorage(dir.path()).load()->get(courier::TrackingId(2)).getStatus() ==
            courier::PackageStatus::Delivered);
}

TEST_CASE("ConsoleApp refuses a status step that is not allowed") {
    const TempDir dir;
    saveTwoShipments(dir.path());
    const Result result = runSession(dir.path(), "7\nCR00000001\n3\n0\n");
    REQUIRE(contains(result.shown,
                     "Not allowed: Cannot change CR00000001 from Accepted to "
                     "Delivered\n"));
    REQUIRE(CsvStorage(dir.path()).load()->get(courier::TrackingId(1)).getStatus() ==
            courier::PackageStatus::Accepted);
}

TEST_CASE("ConsoleApp shows no dispatch alerts before any status change") {
    const TempDir dir;
    REQUIRE(contains(runSession(dir.path(), "8\n0\n").shown, "  (none)\n"));
}

TEST_CASE("ConsoleApp reroutes a shipment, keeping what the user does not change") {
    const TempDir dir;
    saveTwoShipments(dir.path());
    const Result result = runSession(dir.path(), "9\nCR00000001\nPlovdiv\n\n0\n");
    REQUIRE(contains(result.shown, "Destination [Sofia]: Recipient [Ivan Petrov]: "));
    REQUIRE(contains(result.shown, "Done: Reroute CR00000001: Sofia -> Plovdiv\n"));
    const auto saved = CsvStorage(dir.path()).load();
    REQUIRE(saved->get(courier::TrackingId(1)).getDestination() == "Plovdiv");
    REQUIRE(saved->get(courier::TrackingId(1)).getRecipient() == "Ivan Petrov");
}

TEST_CASE("ConsoleApp changes the recipient when rerouting") {
    const TempDir dir;
    saveTwoShipments(dir.path());
    const Result result = runSession(dir.path(), "9\nCR00000001\n\nGeorgi Dimitrov\n0\n");
    REQUIRE(contains(result.shown, "; recipient Ivan Petrov -> Georgi Dimitrov\n"));
}

TEST_CASE("ConsoleApp skips a reroute or modify that changes nothing") {
    const TempDir dir;
    saveTwoShipments(dir.path());
    const Result result =
        runSession(dir.path(), "9\nCR00000001\n\n\n10\nCR00000001\n\n\n\n\n\n13\n0\n");
    const std::string& shown = result.shown;
    const auto first = shown.find("Nothing changed.\n");
    REQUIRE(first != std::string::npos);
    REQUIRE(shown.find("Nothing changed.\n", first + 1) != std::string::npos);
    REQUIRE(contains(shown, "Nothing to undo.\n"));
    REQUIRE(CsvStorage(dir.path()).load()->history().size() == 2);
}

TEST_CASE("ConsoleApp modifies a shipment and shows the new total") {
    const TempDir dir;
    saveTwoShipments(dir.path());
    const Result result = runSession(dir.path(), "10\nCR00000001\n4\n\n\n\n100\n0\n");
    REQUIRE(contains(result.shown, "Weight in kg (up to 1000) [2]: "));
    REQUIRE(contains(result.shown, "Length in cm (up to 500) [30]: "));
    REQUIRE(contains(result.shown, "Declared value (up to 1000000.00) [0.00]: "));
    REQUIRE(contains(result.shown,
                     "Done: Modify CR00000001: weight 2.00 -> 4.00 kg; declared value 0.00 -> "
                     "100.00\n"));
    REQUIRE(contains(result.shown, "New total: 12.00\n"));
}

TEST_CASE("ConsoleApp undoes and redoes changes") {
    const TempDir dir;
    saveTwoShipments(dir.path());
    const Result result = runSession(dir.path(), "9\nCR00000001\nPlovdiv\n\n13\n14\n14\n13\n0\n");
    REQUIRE(contains(result.shown, "Undone: Reroute CR00000001: Sofia -> Plovdiv\n"));
    REQUIRE(contains(result.shown, "Redone: Reroute CR00000001: Sofia -> Plovdiv\n"));
    REQUIRE(contains(result.shown, "Nothing to redo.\n"));
    const auto saved = CsvStorage(dir.path()).load();
    REQUIRE(saved->get(courier::TrackingId(1)).getDestination() == "Sofia");
    REQUIRE(saved->cancelledOperations().size() == 2);
}

TEST_CASE("ConsoleApp refuses to change a delivered shipment") {
    const TempDir dir;
    saveTwoShipments(dir.path());
    const Result result =
        runSession(dir.path(), "7\nCR00000002\n2\n7\nCR00000002\n3\n9\nCR00000002\nBurgas\n\n0\n");
    REQUIRE(
        contains(result.shown,
                 "Not allowed: Shipment CR00000002 is Delivered and can no longer be changed\n"));
}

TEST_CASE("ConsoleApp registers couriers and offices and lists them with their shipments") {
    const TempDir dir;
    saveTwoShipments(dir.path());
    const Result result =
        runSession(dir.path(),
                   "15\nc-01\nPetar Stoyanov\n0888 123 456\n16\nvar-1\nVarna Center\n\n"
                   "11\nCR00000002\nvar-1\n17\n0\n");
    REQUIRE(contains(result.shown, "Phone [none]: "));
    REQUIRE(
        contains(result.shown, "Registered: Courier C-01: Petar Stoyanov (phone 0888 123 456)\n"));
    REQUIRE(contains(result.shown, "Registered: Office VAR-1: Varna Center\n"));
    REQUIRE(contains(result.shown, "Done: Assign CR00000002: none -> VAR-1\n"));
    REQUIRE(
        contains(result.shown, "\nCourier C-01: Petar Stoyanov (phone 0888 123 456)\n  (none)\n"));
    REQUIRE(contains(result.shown, "\nOffice VAR-1: Varna Center\nTracking ID"));
    REQUIRE(contains(result.shown, "CR00000002  Express   Accepted    VAR-1"));

    const auto saved = CsvStorage(dir.path()).load();
    REQUIRE(saved->assigneeCount() == 2);
    REQUIRE(saved->get(courier::TrackingId(2)).getAssigneeCode() == "VAR-1");
}

TEST_CASE("ConsoleApp rejects a bad or duplicate code") {
    const TempDir dir;
    const Result result =
        runSession(dir.path(), "15\nC 01\nPetar\n\n15\nC-01\nPetar\n\n15\nc-01\nIvan\n\n0\n");
    REQUIRE(contains(result.shown, "Rejected: Invalid code \"C 01\""));
    REQUIRE(contains(result.shown, "Rejected: Code C-01 is already in use\n"));
    REQUIRE(CsvStorage(dir.path()).load()->assigneeCount() == 1);
}

TEST_CASE("ConsoleApp reports an unknown courier or office") {
    const TempDir dir;
    saveTwoShipments(dir.path());
    const Result result = runSession(dir.path(), "11\nCR00000001\nX-9\n0\n");
    REQUIRE(contains(result.shown, "Not found: No courier or office with code X-9\n"));
}

TEST_CASE("ConsoleApp unassigns a shipment and skips one that is not assigned") {
    const TempDir dir;
    saveTwoShipments(dir.path());
    const Result result =
        runSession(dir.path(),
                   "15\nC-01\nPetar\n\n11\nCR00000001\nC-01\n11\nCR00000001\nc-01\n12\nCR00000001\n"
                   "12\nCR00000001\n0\n");
    REQUIRE(contains(result.shown, "Assigned to: C-01\n"));
    REQUIRE(contains(result.shown, "Nothing changed.\n"));
    REQUIRE(contains(result.shown, "Done: Assign CR00000001: C-01 -> none\n"));
    REQUIRE(contains(result.shown, "CR00000001 is not assigned to anyone.\n"));
    REQUIRE_FALSE(CsvStorage(dir.path()).load()->get(courier::TrackingId(1)).getAssigneeCode());
}

TEST_CASE("ConsoleApp shows the history, including earlier sessions") {
    const TempDir dir;
    saveTwoShipments(dir.path());
    const Result result = runSession(dir.path(), "9\nCR00000001\nPlovdiv\n\n13\n18\n19\n0\n");
    const std::string& shown = result.shown;
    const auto history = shown.find("Oldest first:\n");
    const auto cancelled = shown.find("Every undo made, oldest first:\n");
    REQUIRE(history != std::string::npos);
    REQUIRE(cancelled != std::string::npos);
    const std::string historyPart = shown.substr(history, cancelled - history);
    REQUIRE(
        contains(historyPart, "  Accept CR00000001: Standard shipment to Sofia for Ivan Petrov\n"));
    REQUIRE(contains(historyPart, "  Accept CR00000002: Express shipment to Varna"));
    REQUIRE(contains(historyPart, "  Reroute CR00000001: Sofia -> Plovdiv\n"));
    REQUIRE(contains(historyPart, "  Undo: Reroute CR00000001: Sofia -> Plovdiv\n"));
    REQUIRE(historyPart.find("Accept CR00000002") < historyPart.find("Reroute CR00000001"));
    const std::string cancelledPart = shown.substr(cancelled);
    REQUIRE(contains(cancelledPart, "  Undo: Reroute CR00000001: Sofia -> Plovdiv\n"));
    REQUIRE_FALSE(contains(cancelledPart, "Accept CR"));
}

TEST_CASE("ConsoleApp shows empty history lists as none") {
    const TempDir dir;
    const std::string shown = runSession(dir.path(), "18\n19\n0\n").shown;
    REQUIRE(contains(shown, "Oldest first:\n  (none)\n"));
    REQUIRE(contains(shown, "Every undo made, oldest first:\n  (none)\n"));
}
