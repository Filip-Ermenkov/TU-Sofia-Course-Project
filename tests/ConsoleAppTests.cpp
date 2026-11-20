#include <catch2/catch_test_macros.hpp>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <system_error>

#include "courier/ConsoleApp.hpp"
#include "courier/CsvStorage.hpp"
#include "courier/Dimensions.hpp"
#include "courier/LogisticsManager.hpp"
#include "courier/ServiceType.hpp"

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
    REQUIRE(contains(result.shown, "Shipments\n   2  Find a shipment by tracking ID\n"));
    REQUIRE(contains(result.shown, "   0  Exit\n"));
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
    const Result result = runSession(dir.path(), "3\n0\n");
    REQUIRE(contains(result.shown, "Loaded 2 shipments, 0 couriers and offices and 2 history"));
    REQUIRE(contains(result.shown, "2 shipments (2 waiting)"));
    REQUIRE(contains(result.shown, "CR00000001  Standard  Accepted    -           Sofia"));
    REQUIRE(contains(result.shown, "CR00000002  Express   Accepted    -           Varna"));
}

TEST_CASE("ConsoleApp lists no shipments as none") {
    const TempDir dir;
    REQUIRE(contains(runSession(dir.path(), "3\n0\n").shown, "  (none)\n"));
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

    const Result accepted = runSession(dir.path(), "y\n3\n0\n");
    REQUIRE(accepted.exitCode == EXIT_SUCCESS);
    REQUIRE(contains(accepted.shown, "0 shipments (0 waiting)"));
    REQUIRE(readFile(file) == "not a header\n");  // replaced only when a change is saved
}
