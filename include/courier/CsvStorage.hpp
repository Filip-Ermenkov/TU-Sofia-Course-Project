#pragma once

#include <filesystem>
#include <memory>

#include "courier/LogisticsManager.hpp"

namespace courier {

// Keeps the whole state of a LogisticsManager in three CSV files in one folder: the couriers and
// offices (assignees.csv), the shipments with their status and assignment (shipments.csv) and
// the full history of logistics operations (history.csv). Text in the files is written as is
// (UTF-8 stays UTF-8); numbers do not depend on the locale.
class CsvStorage {
public:
    static constexpr const char* kAssigneesFile = "assignees.csv";
    static constexpr const char* kShipmentsFile = "shipments.csv";
    static constexpr const char* kHistoryFile = "history.csv";

    // Uses the folder "data" in the current working directory.
    CsvStorage();
    explicit CsvStorage(std::filesystem::path directory);

    [[nodiscard]] const std::filesystem::path& getDirectory() const;

    // Writes everything, creating the folder if needed. Each file is first written under a
    // temporary name and only then put in place of the old one, so a failed write leaves the
    // saved data as it was. Throws StorageException on failure.
    void save(const LogisticsManager& manager) const;

    // Reads everything back into a new manager: couriers and offices first, then the shipments
    // (whose courier or office must exist), then the history (whose shipments must exist).
    // Numbering continues after the highest saved tracking ID. Loading adds no history records
    // and, as the manager is new, notifies no observer. If none of the files exists yet, the
    // manager is empty. Throws StorageException, naming the file and line where possible, if only
    // some of the files exist or anything in them is wrong; nothing the caller has is changed.
    [[nodiscard]] std::unique_ptr<LogisticsManager> load() const;

private:
    std::filesystem::path directory_;
};

}  // namespace courier
