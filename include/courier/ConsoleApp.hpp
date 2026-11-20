#pragma once

#include <chrono>
#include <functional>
#include <istream>
#include <memory>
#include <ostream>
#include <string_view>
#include <vector>

#include "courier/CsvStorage.hpp"
#include "courier/DispatchModule.hpp"
#include "courier/InputReader.hpp"
#include "courier/LogisticsManager.hpp"
#include "courier/Package.hpp"

namespace courier {

// The console program: loads the saved data, shows the menu, runs what the user picks and saves
// after every change. Errors from the logistics classes are shown as messages and the menu comes
// back; nothing a failed action touched has changed.
class ConsoleApp {
public:
    // Talks to the user through std::cin and std::cout and keeps the data in the folder "data".
    ConsoleApp();
    // Both streams must outlive the app.
    ConsoleApp(std::istream& in, std::ostream& out, CsvStorage storage);

    // Owns the manager and the input of the session, so it is neither copied nor moved.
    ConsoleApp(const ConsoleApp&) = delete;
    ConsoleApp& operator=(const ConsoleApp&) = delete;
    ConsoleApp(ConsoleApp&&) = delete;
    ConsoleApp& operator=(ConsoleApp&&) = delete;
    ~ConsoleApp() = default;

    // Runs until the user chooses Exit or the input ends. Returns EXIT_SUCCESS, or EXIT_FAILURE if
    // the saved data could not be loaded or the last changes could not be saved.
    int run();

private:
    // One line of the menu. The action returns true if it changed data that must be saved.
    struct MenuItem {
        int key;
        std::string_view section;
        std::string_view label;
        std::function<bool()> action;
    };

    [[nodiscard]] std::vector<MenuItem> buildMenu();
    [[nodiscard]] bool loadData();
    void saveData();
    void showMenu() const;
    // Runs the action, shows any logistics error as a message and saves if data changed.
    void runAction(const MenuItem& item);
    // True if the program may end now; tries to save what is still unsaved first.
    [[nodiscard]] bool confirmExit();
    // Says goodbye and returns the exit code of run().
    [[nodiscard]] int finish() const;

    void printPackages(const std::vector<const Package*>& packages) const;

    bool findShipment();
    bool listShipments();

    std::ostream& out_;
    InputReader input_;
    CsvStorage storage_;
    const std::chrono::time_zone* zone_;
    // Declared before the manager, so it is destroyed after the manager that points to it.
    DispatchModule dispatch_;
    std::unique_ptr<LogisticsManager> manager_;
    std::vector<MenuItem> menu_;
    bool unsaved_ = false;
};

}  // namespace courier
