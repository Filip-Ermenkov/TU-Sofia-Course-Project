#include "courier/CsvWriter.hpp"

#include <cstddef>
#include <format>
#include <stdexcept>
#include <string_view>
#include <utility>

#include "courier/StorageException.hpp"

namespace courier {

namespace {

std::string quoteIfNeeded(std::string_view field) {
    if (field.find_first_of(",\"\r\n") == std::string_view::npos) {
        return std::string(field);
    }
    std::string quoted = "\"";
    for (const char c : field) {
        if (c == '"') {
            quoted += '"';
        }
        quoted += c;
    }
    quoted += '"';
    return quoted;
}

}  // namespace

CsvWriter::CsvWriter(std::ostream& out) : CsvWriter(out, "output") {}

CsvWriter::CsvWriter(std::ostream& out, std::string targetName)
    : out_(out), targetName_(std::move(targetName)) {}

const std::string& CsvWriter::getTargetName() const { return targetName_; }

void CsvWriter::writeRow(const std::vector<std::string>& fields) {
    if (fields.empty()) {
        throw std::invalid_argument("CsvWriter::writeRow needs at least one field");
    }
    std::string line;
    // A lone empty field would give an empty line, which reads as nothing; quotes keep it visible.
    if (fields.size() == 1 && fields.front().empty()) {
        line = "\"\"";
    } else {
        for (std::size_t i = 0; i < fields.size(); ++i) {
            if (i > 0) {
                line += ',';
            }
            line += quoteIfNeeded(fields[i]);
        }
    }
    line += '\n';
    out_ << line;
    if (!out_) {
        throw StorageException(std::format("Cannot write to {}", targetName_));
    }
}

}  // namespace courier
