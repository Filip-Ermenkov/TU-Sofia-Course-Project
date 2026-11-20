#include "courier/CsvStorage.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <format>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "courier/AssigneeKind.hpp"
#include "courier/Courier.hpp"
#include "courier/CsvReader.hpp"
#include "courier/CsvWriter.hpp"
#include "courier/Dimensions.hpp"
#include "courier/Money.hpp"
#include "courier/Office.hpp"
#include "courier/OperationRecord.hpp"
#include "courier/OperationType.hpp"
#include "courier/PackageStatus.hpp"
#include "courier/ParcelDetails.hpp"
#include "courier/ServiceType.hpp"
#include "courier/StorageException.hpp"
#include "courier/TrackingId.hpp"
#include "courier/ValidationException.hpp"

namespace courier {

namespace {

namespace fs = std::filesystem;

constexpr std::array<std::string_view, 4> kAssigneesHeader{"kind", "code", "name", "contact"};
constexpr std::array<std::string_view, 11> kShipmentsHeader{
    "id",        "service",     "status",    "weight_kg",
    "length_cm", "width_cm",    "height_cm", "declared_value_cents",
    "recipient", "destination", "assignee"};
constexpr std::array<std::string_view, 4> kHistoryHeader{"time", "type", "id", "details"};

void writeHeader(CsvWriter& writer, std::span<const std::string_view> header) {
    writer.writeRow(std::vector<std::string>(header.begin(), header.end()));
}

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
    writeHeader(writer, kAssigneesHeader);
    for (const Assignee* assignee : manager.assignees()) {
        writer.writeRow({std::string(toString(assignee->kind())), assignee->getCode(),
                         assignee->getName(), contactOf(*assignee)});
    }
}

void writeShipments(CsvWriter& writer, const LogisticsManager& manager) {
    writeHeader(writer, kShipmentsHeader);
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
    writeHeader(writer, kHistoryHeader);
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

// The readers below throw ValidationException for a bad field; readFile() adds file and line.

double toDouble(const std::string& field, std::string_view column) {
    double value = 0.0;
    const char* const end = field.data() + field.size();
    const auto [stop, error] = std::from_chars(field.data(), end, value);
    if (field.empty() || error != std::errc{} || stop != end) {
        throw ValidationException(std::format("Invalid {} \"{}\"", column, field));
    }
    return value;
}

std::int64_t toInteger(const std::string& field, std::string_view column) {
    std::int64_t value = 0;
    const char* const end = field.data() + field.size();
    const auto [stop, error] = std::from_chars(field.data(), end, value);
    if (field.empty() || error != std::errc{} || stop != end) {
        throw ValidationException(std::format("Invalid {} \"{}\"", column, field));
    }
    return value;
}

// The result of one of the parse... functions, or an error naming the column.
template <typename Enum>
Enum toEnum(std::optional<Enum> parsed, const std::string& field, std::string_view column) {
    if (!parsed) {
        throw ValidationException(std::format("Unknown {} \"{}\"", column, field));
    }
    return *parsed;
}

using Row = std::vector<std::string>;

void readAssignee(const Row& row, LogisticsManager& manager) {
    const std::string& code = row[1];
    const std::string& name = row[2];
    const std::string& contact = row[3];
    std::unique_ptr<Assignee> assignee;
    switch (toEnum(parseAssigneeKind(row[0]), row[0], "kind")) {
        case AssigneeKind::Courier:
            assignee = contact.empty() ? std::make_unique<Courier>(code, name)
                                       : std::make_unique<Courier>(code, name, contact);
            break;
        case AssigneeKind::Office:
            assignee = contact.empty() ? std::make_unique<Office>(code, name)
                                       : std::make_unique<Office>(code, name, contact);
            break;
    }
    manager.addAssignee(std::move(assignee));
}

void readShipment(const Row& row, LogisticsManager& manager) {
    const ParcelDetails parcel(
        toDouble(row[3], "weight_kg"),
        Dimensions(toDouble(row[4], "length_cm"), toDouble(row[5], "width_cm"),
                   toDouble(row[6], "height_cm")),
        Money(toInteger(row[7], "declared_value_cents")));
    std::optional<std::string> assignee;
    if (!row[10].empty()) {
        assignee = row[10];
    }
    manager.restore(toEnum(parseServiceType(row[1]), row[1], "service"), TrackingId(row[0]), parcel,
                    row[8], row[9], toEnum(parsePackageStatus(row[2]), row[2], "status"),
                    std::move(assignee));
}

void readRecord(const Row& row, LogisticsManager& manager) {
    const std::chrono::sys_seconds time{std::chrono::seconds{toInteger(row[0], "time")}};
    manager.restoreRecord(OperationRecord(toEnum(parseOperationType(row[1]), row[1], "type"),
                                          TrackingId(row[2]), row[3], time));
}

// Checks the header, then hands every record to readRow. Any error inside a record becomes a
// StorageException that names the file and the line.
void readFile(const fs::path& path, const char* name, std::span<const std::string_view> header,
              const std::function<void(const Row&)>& readRow) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw StorageException(std::format("Cannot open {} for reading", path.string()));
    }
    CsvReader reader(in, name);
    const auto first = reader.readRow();
    if (!first || !std::ranges::equal(*first, header)) {
        throw StorageException(name, 1, "unexpected header");
    }
    while (const auto row = reader.readRow()) {
        if (row->size() != header.size()) {
            throw StorageException(
                name, reader.getRowLine(),
                std::format("expected {} fields, found {}", header.size(), row->size()));
        }
        try {
            readRow(*row);
        } catch (const LogisticsException& error) {
            throw StorageException(name, reader.getRowLine(), error.what());
        }
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

std::unique_ptr<LogisticsManager> CsvStorage::load() const {
    std::size_t found = 0;
    for (const char* name : {kAssigneesFile, kShipmentsFile, kHistoryFile}) {
        std::error_code error;
        const bool exists = fs::exists(directory_ / name, error);
        if (error) {
            throw StorageException(
                std::format("Cannot check {}: {}", (directory_ / name).string(), error.message()));
        }
        if (exists) {
            ++found;
        }
    }
    auto manager = std::make_unique<LogisticsManager>();
    if (found == 0) {
        return manager;  // first run: nothing saved yet
    }
    if (found < 3) {
        throw StorageException(
            std::format("Saved data in {} is incomplete: {}, {} and {} must all exist",
                        directory_.string(), kAssigneesFile, kShipmentsFile, kHistoryFile));
    }
    // This order, because shipments refer to assignees and records refer to shipments.
    readFile(directory_ / kAssigneesFile, kAssigneesFile, kAssigneesHeader,
             [&manager](const Row& row) { readAssignee(row, *manager); });
    readFile(directory_ / kShipmentsFile, kShipmentsFile, kShipmentsHeader,
             [&manager](const Row& row) { readShipment(row, *manager); });
    readFile(directory_ / kHistoryFile, kHistoryFile, kHistoryHeader,
             [&manager](const Row& row) { readRecord(row, *manager); });
    return manager;
}

}  // namespace courier
