#include "courier/CsvStorage.hpp"

#include <array>
#include <format>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "courier/Courier.hpp"
#include "courier/CsvWriter.hpp"
#include "courier/Office.hpp"
#include "courier/StorageException.hpp"

namespace courier {

namespace {

namespace fs = std::filesystem;

// The phone of a courier or the address of an office; empty if none was given.
std::string contactOf(const Assignee& assignee) {
    switch (assignee.kind()) {
        case AssigneeKind::Courier:
            return dynamic_cast<const Courier&>(assignee).getPhone();
        case AssigneeKind::Office:
            return dynamic_cast<const Office&>(assignee).getAddress();
    }
    // Only reachable if an invalid value was forced in with static_cast.
    throw std::logic_error("Unknown AssigneeKind");
}

void writeAssignees(CsvWriter& writer, const LogisticsManager& manager) {
    writer.writeRow({"kind", "code", "name", "contact"});
    for (const Assignee* assignee : manager.assignees()) {
        writer.writeRow({std::string(toString(assignee->kind())), assignee->getCode(),
                         assignee->getName(), contactOf(*assignee)});
    }
}

void writeShipments(CsvWriter& writer, const LogisticsManager& manager) {
    writer.writeRow({"id", "service", "status", "weight_kg", "length_cm", "width_cm", "height_cm",
                     "declared_value_cents", "recipient", "destination", "assignee"});
    // "{}" writes the shortest text that reads back as exactly the same double.
    for (const Package* package : manager.filter([](const Package&) { return true; })) {
        const Dimensions& size = package->getDimensions();
        writer.writeRow(
            {package->getId().getValue(), std::string(toString(package->serviceType())),
             std::string(toString(package->getStatus())), std::format("{}", package->getWeightKg()),
             std::format("{}", size.getLength()), std::format("{}", size.getWidth()),
             std::format("{}", size.getHeight()),
             std::format("{}", package->getDeclaredValue().getCents()), package->getRecipient(),
             package->getDestination(), package->getAssigneeCode().value_or("")});
    }
}

void writeHistory(CsvWriter& writer, const LogisticsManager& manager) {
    writer.writeRow({"time", "type", "id", "details"});
    for (const OperationRecord& record : manager.history()) {
        writer.writeRow({std::format("{}", record.getTime().time_since_epoch().count()),
                         std::string(toString(record.getType())), record.getId().getValue(),
                         record.getDetails()});
    }
}

// Binary mode, so "\n" is written as it is on every system.
void writeFile(const fs::path& path, const std::string& name,
               const std::function<void(CsvWriter&)>& fill) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        throw StorageException(std::format("Cannot open {} for writing", path.string()));
    }
    CsvWriter writer(out, name);
    fill(writer);
    out.close();
    if (!out) {
        throw StorageException(std::format("Cannot write to {}", path.string()));
    }
}

}  // namespace

CsvStorage::CsvStorage() : CsvStorage("data") {}

CsvStorage::CsvStorage(std::filesystem::path directory) : directory_(std::move(directory)) {}

const std::filesystem::path& CsvStorage::getDirectory() const { return directory_; }

void CsvStorage::save(const LogisticsManager& manager) const {
    std::error_code error;
    fs::create_directories(directory_, error);
    if (error) {
        throw StorageException(
            std::format("Cannot create folder {}: {}", directory_.string(), error.message()));
    }

    struct File {
        const char* name;
        std::function<void(CsvWriter&)> fill;
    };
    const std::array<File, 3> files{
        File{kAssigneesFile, [&manager](CsvWriter& w) { writeAssignees(w, manager); }},
        File{kShipmentsFile, [&manager](CsvWriter& w) { writeShipments(w, manager); }},
        File{kHistoryFile, [&manager](CsvWriter& w) { writeHistory(w, manager); }}};
    const auto temporary = [this](const File& file) {
        return directory_ / (std::string(file.name) + ".tmp");
    };

    try {
        for (const File& file : files) {
            writeFile(temporary(file), file.name, file.fill);
        }
        // Every file is complete; only now are the old ones replaced.
        for (const File& file : files) {
            fs::rename(temporary(file), directory_ / file.name, error);
            if (error) {
                throw StorageException(std::format(
                    "Cannot replace {}: {}", (directory_ / file.name).string(), error.message()));
            }
        }
    } catch (...) {
        for (const File& file : files) {
            fs::remove(temporary(file), error);  // best effort; the first error is reported
        }
        throw;
    }
}

}  // namespace courier
