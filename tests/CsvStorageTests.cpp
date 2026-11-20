#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "courier/Courier.hpp"
#include "courier/CsvStorage.hpp"
#include "courier/LogisticsManager.hpp"
#include "courier/Office.hpp"
#include "courier/StorageException.hpp"

using courier::CsvStorage;
using courier::Dimensions;
using courier::LogisticsManager;
using courier::Money;
using courier::PackageStatus;
using courier::ServiceType;
using courier::StorageException;
namespace fs = std::filesystem;

namespace {

// A fresh folder under the system's temporary directory, deleted with everything in it at the end.
class TempDir {
public:
    TempDir()
        : path_(fs::temp_directory_path() /
                ("courier-tests-" + std::to_string(std::random_device{}()))) {
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

std::string readFile(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream text;
    text << in.rdbuf();
    return text.str();
}

void writeFile(const fs::path& path, const std::string& text) {
    std::ofstream out(path, std::ios::binary);
    out << text;
}

// Two assignees and three shipments with every kind of field filled in.
void fill(LogisticsManager& manager) {
    manager.addAssignee(
        std::make_unique<courier::Courier>("C-01", "Petar Stoyanov", "0888 123 456"));
    manager.addAssignee(std::make_unique<courier::Office>("PLD-1", "Plovdiv Center",
                                                          "12 \"Ivan Vazov\" St, Plovdiv"));
    manager.addAssignee(std::make_unique<courier::Courier>("C-02", "Nikolay Todorov"));
    const auto& first = manager.accept(ServiceType::Standard, 2.0, Dimensions(30.0, 20.0, 10.0),
                                       "Ivan Petrov", "Sofia, 1 Vitosha Blvd");
    const auto& second = manager.accept(ServiceType::Fragile, 1.25, Dimensions(12.5), "Maria",
                                        "Plovdiv", Money(20050));
    manager.accept(ServiceType::Express, 0.1, Dimensions(10.0, 10.0, 0.5), "Georgi", "Varna");
    manager.assign(first.getId(), "C-01");
    manager.assign(second.getId(), "PLD-1");
    manager.changeStatus(first.getId(), PackageStatus::InTransit);
    manager.reroute(second.getId(), "Plovdiv, Line 1\nLine 2");
    manager.undo();
}

}  // namespace

TEST_CASE("CsvStorage uses the data folder unless told otherwise") {
    REQUIRE(CsvStorage().getDirectory() == fs::path("data"));
    REQUIRE(CsvStorage(fs::path("saves")).getDirectory() == fs::path("saves"));
}

TEST_CASE("CsvStorage saves couriers and offices") {
    const TempDir dir;
    LogisticsManager manager;
    fill(manager);
    CsvStorage(dir.path()).save(manager);
    REQUIRE(readFile(dir.path() / CsvStorage::kAssigneesFile) ==
            "kind,code,name,contact\n"
            "Courier,C-01,Petar Stoyanov,0888 123 456\n"
            "Courier,C-02,Nikolay Todorov,\n"
            "Office,PLD-1,Plovdiv Center,\"12 \"\"Ivan Vazov\"\" St, Plovdiv\"\n");
}

TEST_CASE("CsvStorage saves shipments with their status and assignment") {
    const TempDir dir;
    LogisticsManager manager;
    fill(manager);
    CsvStorage(dir.path()).save(manager);
    REQUIRE(
        readFile(dir.path() / CsvStorage::kShipmentsFile) ==
        "id,service,status,weight_kg,length_cm,width_cm,height_cm,declared_value_cents,"
        "recipient,destination,assignee\n"
        "CR00000001,Standard,In transit,2,30,20,10,0,Ivan Petrov,\"Sofia, 1 Vitosha Blvd\",C-01\n"
        "CR00000002,Fragile,Accepted,1.25,12.5,12.5,12.5,20050,Maria,Plovdiv,PLD-1\n"
        "CR00000003,Express,Accepted,0.1,10,10,0.5,0,Georgi,Varna,\n");
}

TEST_CASE("CsvStorage saves the whole history with times in seconds") {
    const TempDir dir;
    LogisticsManager manager;
    fill(manager);
    CsvStorage(dir.path()).save(manager);

    const std::vector<std::string> rows{
        "Accept,CR00000001,\"Standard shipment to Sofia, 1 Vitosha Blvd for Ivan Petrov\"",
        "Accept,CR00000002,Fragile shipment to Plovdiv for Maria",
        "Accept,CR00000003,Express shipment to Varna for Georgi",
        "Assign,CR00000001,Assign CR00000001: none -> C-01",
        "Assign,CR00000002,Assign CR00000002: none -> PLD-1",
        "Status change,CR00000001,Accepted -> In transit",
        "Reroute,CR00000002,\"Reroute CR00000002: Plovdiv -> Plovdiv, Line 1\nLine 2\"",
        "Undo,CR00000002,\"Reroute CR00000002: Plovdiv -> Plovdiv, Line 1\nLine 2\""};
    const auto& history = manager.history();
    REQUIRE(history.size() == rows.size());
    std::string expected = "time,type,id,details\n";
    for (std::size_t i = 0; i < rows.size(); ++i) {
        expected +=
            std::format("{},{}\n", history[i].getTime().time_since_epoch().count(), rows[i]);
    }
    REQUIRE(readFile(dir.path() / CsvStorage::kHistoryFile) == expected);
}

TEST_CASE("CsvStorage saves an empty manager as headers only") {
    const TempDir dir;
    const LogisticsManager manager;
    CsvStorage(dir.path()).save(manager);
    REQUIRE(readFile(dir.path() / CsvStorage::kAssigneesFile) == "kind,code,name,contact\n");
    REQUIRE(readFile(dir.path() / CsvStorage::kHistoryFile) == "time,type,id,details\n");
    REQUIRE(readFile(dir.path() / CsvStorage::kShipmentsFile).starts_with("id,service,"));
}

TEST_CASE("CsvStorage creates the folder and replaces earlier saves") {
    const TempDir dir;
    const CsvStorage storage(dir.path() / "nested" / "data");
    LogisticsManager manager;
    manager.accept(ServiceType::Standard, 1.0, Dimensions(10.0), "A", "Sofia");
    storage.save(manager);
    manager.accept(ServiceType::Express, 1.0, Dimensions(10.0), "B", "Varna");
    storage.save(manager);

    REQUIRE(readFile(storage.getDirectory() / CsvStorage::kShipmentsFile).find("CR00000002") !=
            std::string::npos);
    std::size_t fileCount = 0;
    for (const auto& entry : fs::directory_iterator(storage.getDirectory())) {
        REQUIRE(entry.path().extension() == ".csv");  // no temporary file is left behind
        ++fileCount;
    }
    REQUIRE(fileCount == 3);
}

TEST_CASE("CsvStorage reports a folder it cannot create") {
    const TempDir dir;
    writeFile(dir.path() / "taken", "a file, not a folder");
    const LogisticsManager manager;
    REQUIRE_THROWS_AS(CsvStorage(dir.path() / "taken").save(manager), StorageException);
}

TEST_CASE("CsvStorage keeps no temporary files when a save fails") {
    const TempDir dir;
    // A folder where the shipments file should be makes replacing it fail.
    fs::create_directories(dir.path() / CsvStorage::kShipmentsFile / "inside");
    const LogisticsManager manager;
    REQUIRE_THROWS_AS(CsvStorage(dir.path()).save(manager), StorageException);
    for (const auto& entry : fs::directory_iterator(dir.path())) {
        REQUIRE(entry.path().extension() != ".tmp");
    }
}

namespace {

const std::string kAssigneesHeader = "kind,code,name,contact\n";
const std::string kShipmentsHeader =
    "id,service,status,weight_kg,length_cm,width_cm,height_cm,declared_value_cents,recipient,"
    "destination,assignee\n";
const std::string kHistoryHeader = "time,type,id,details\n";

// A small valid save, which each test below breaks in one place.
struct SavedFiles {
    std::string assignees = kAssigneesHeader + "Courier,C-01,Ivan,\n";
    std::string shipments =
        kShipmentsHeader + "CR00000001,Standard,Accepted,1,10,10,10,0,A,Sofia,C-01\n";
    std::string history = kHistoryHeader + "1700000000,Accept,CR00000001,Standard shipment\n";
};

// The message of the StorageException thrown while loading the files.
std::string loadError(const SavedFiles& files) {
    const TempDir dir;
    writeFile(dir.path() / CsvStorage::kAssigneesFile, files.assignees);
    writeFile(dir.path() / CsvStorage::kShipmentsFile, files.shipments);
    writeFile(dir.path() / CsvStorage::kHistoryFile, files.history);
    try {
        static_cast<void>(CsvStorage(dir.path()).load());
    } catch (const StorageException& error) {
        return error.what();
    }
    return "no error";
}

}  // namespace

TEST_CASE("CsvStorage loads back exactly what it saved") {
    const TempDir dir;
    LogisticsManager original;
    fill(original);
    const CsvStorage storage(dir.path());
    storage.save(original);
    const auto loaded = storage.load();

    REQUIRE(loaded->assigneeCount() == original.assigneeCount());
    for (const courier::Assignee* assignee : original.assignees()) {
        const courier::Assignee& copy = loaded->getAssignee(assignee->getCode());
        REQUIRE(copy.kind() == assignee->kind());
        REQUIRE(copy.describe() == assignee->describe());
    }

    REQUIRE(loaded->packageCount() == original.packageCount());
    for (const courier::Package* package : original.filter([](const auto&) { return true; })) {
        const courier::Package& copy = loaded->get(package->getId());
        REQUIRE(copy.serviceType() == package->serviceType());
        REQUIRE(copy.getStatus() == package->getStatus());
        REQUIRE(copy.getWeightKg() == package->getWeightKg());
        REQUIRE(copy.getDimensions().getLength() == package->getDimensions().getLength());
        REQUIRE(copy.getDimensions().getWidth() == package->getDimensions().getWidth());
        REQUIRE(copy.getDimensions().getHeight() == package->getDimensions().getHeight());
        REQUIRE(copy.getDeclaredValue() == package->getDeclaredValue());
        REQUIRE(copy.getRecipient() == package->getRecipient());
        REQUIRE(copy.getDestination() == package->getDestination());
        REQUIRE(copy.getAssigneeCode() == package->getAssigneeCode());
        REQUIRE(copy.totalCost() == package->totalCost());
    }

    REQUIRE(loaded->history().size() == original.history().size());
    for (std::size_t i = 0; i < original.history().size(); ++i) {
        REQUIRE(loaded->history()[i].getType() == original.history()[i].getType());
        REQUIRE(loaded->history()[i].getId() == original.history()[i].getId());
        REQUIRE(loaded->history()[i].getDetails() == original.history()[i].getDetails());
        REQUIRE(loaded->history()[i].getTime() == original.history()[i].getTime());
    }
    REQUIRE(loaded->cancelledOperations().size() == 1);
    REQUIRE_FALSE(loaded->canUndo());  // undo stacks are not saved
}

TEST_CASE("CsvStorage continues numbering and the delivery queue after loading") {
    const TempDir dir;
    LogisticsManager original;
    fill(original);
    const CsvStorage storage(dir.path());
    storage.save(original);
    const auto loaded = storage.load();

    REQUIRE(loaded->accept(ServiceType::Standard, 1.0, Dimensions(10.0), "New", "Ruse").getId() ==
            courier::TrackingId(4));
    REQUIRE(loaded->dispatchNext() == courier::TrackingId(3));  // the express one first
    REQUIRE(loaded->dispatchNext() == courier::TrackingId(2));
    REQUIRE(loaded->dispatchNext() == courier::TrackingId(4));
    REQUIRE_FALSE(loaded->dispatchNext().has_value());
}

TEST_CASE("CsvStorage gives an empty manager when nothing was saved yet") {
    const TempDir dir;
    const auto loaded = CsvStorage(dir.path() / "never-saved").load();
    REQUIRE(loaded->packageCount() == 0);
    REQUIRE(loaded->assigneeCount() == 0);
    REQUIRE(loaded->history().empty());
}

TEST_CASE("CsvStorage refuses incomplete saved data") {
    const TempDir dir;
    const CsvStorage storage(dir.path());
    storage.save(LogisticsManager());
    fs::remove(dir.path() / CsvStorage::kHistoryFile);
    REQUIRE_THROWS_AS(storage.load(), StorageException);
}

TEST_CASE("CsvStorage loads the hand-made valid save") {
    REQUIRE(loadError(SavedFiles{}) == "no error");
}

TEST_CASE("CsvStorage names the file and line of damaged data") {
    SavedFiles files;
    SECTION("wrong header") {
        files.shipments = "id,service\n";
        REQUIRE(loadError(files) == "shipments.csv line 1: unexpected header");
    }
    SECTION("empty file") {
        files.history = "";
        REQUIRE(loadError(files) == "history.csv line 1: unexpected header");
    }
    SECTION("missing fields") {
        files.shipments = kShipmentsHeader + "CR00000001,Standard,Accepted\n";
        REQUIRE(loadError(files) == "shipments.csv line 2: expected 11 fields, found 3");
    }
    SECTION("broken quoting") {
        files.assignees = kAssigneesHeader + "Courier,C-01,Iv\"an,\n";
        REQUIRE(loadError(files) == "assignees.csv line 2: quote inside an unquoted field");
    }
    SECTION("not a number") {
        files.shipments =
            kShipmentsHeader + "CR00000001,Standard,Accepted,1kg,10,10,10,0,A,Sofia,\n";
        REQUIRE(loadError(files) == "shipments.csv line 2: Invalid weight_kg \"1kg\"");
    }
    SECTION("value out of range") {
        files.shipments =
            kShipmentsHeader + "CR00000001,Standard,Accepted,-1,10,10,10,0,A,Sofia,\n";
        REQUIRE(loadError(files).starts_with("shipments.csv line 2: Invalid weight"));
    }
    SECTION("negative declared value") {
        files.shipments =
            kShipmentsHeader + "CR00000001,Standard,Accepted,1,10,10,10,-5,A,Sofia,\n";
        REQUIRE(loadError(files).starts_with("shipments.csv line 2: "));
    }
    SECTION("unknown service type") {
        files.shipments =
            kShipmentsHeader + "CR00000001,Overnight,Accepted,1,10,10,10,0,A,Sofia,\n";
        REQUIRE(loadError(files) == "shipments.csv line 2: Unknown service \"Overnight\"");
    }
    SECTION("bad tracking ID") {
        files.shipments = kShipmentsHeader + "CR1,Standard,Accepted,1,10,10,10,0,A,Sofia,\n";
        REQUIRE(loadError(files).starts_with("shipments.csv line 2: "));
    }
    SECTION("unknown courier or office") {
        files.shipments =
            kShipmentsHeader + "CR00000001,Standard,Accepted,1,10,10,10,0,A,Sofia,C-09\n";
        REQUIRE(loadError(files) == "shipments.csv line 2: No courier or office with code C-09");
    }
    SECTION("duplicate tracking ID") {
        files.shipments += "CR00000001,Express,Accepted,1,10,10,10,0,B,Varna,\n";
        REQUIRE(loadError(files) ==
                "shipments.csv line 3: Tracking ID CR00000001 is already in use");
    }
    SECTION("duplicate code") {
        files.assignees += "Office,C-01,Other,\n";
        REQUIRE(loadError(files) == "assignees.csv line 3: Code C-01 is already in use");
    }
    SECTION("unknown kind") {
        files.assignees = kAssigneesHeader + "Driver,C-01,Ivan,\n";
        REQUIRE(loadError(files) == "assignees.csv line 2: Unknown kind \"Driver\"");
    }
    SECTION("history of an unknown shipment") {
        files.history += "1700000001,Reroute,CR00000002,Reroute CR00000002: A -> B\n";
        REQUIRE(loadError(files).starts_with("history.csv line 3: "));
    }
    SECTION("bad time") {
        files.history = kHistoryHeader + "17e8,Accept,CR00000001,Standard shipment\n";
        REQUIRE(loadError(files) == "history.csv line 2: Invalid time \"17e8\"");
    }
}
