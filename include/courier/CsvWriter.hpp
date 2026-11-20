#pragma once

#include <ostream>
#include <string>
#include <vector>

namespace courier {

// Writes records in CSV format (RFC 4180), one line per record. A field that contains a comma,
// a quote or a line break is put in quotes, with each quote inside it doubled, so any text can be
// read back exactly. The stream is not owned and must outlive the writer.
class CsvWriter {
public:
    // Error messages call the target "output".
    explicit CsvWriter(std::ostream& out);
    // targetName appears in error messages, e.g. the name of the file being written.
    CsvWriter(std::ostream& out, std::string targetName);

    // Bound to one stream for its whole life, so it cannot be copied or reassigned.
    CsvWriter(const CsvWriter&) = delete;
    CsvWriter& operator=(const CsvWriter&) = delete;
    CsvWriter(CsvWriter&&) = delete;
    CsvWriter& operator=(CsvWriter&&) = delete;
    ~CsvWriter() = default;

    [[nodiscard]] const std::string& getTargetName() const;

    // Writes one record ending in '\n'. Throws std::invalid_argument for a record with no fields
    // and StorageException if the stream fails.
    void writeRow(const std::vector<std::string>& fields);

private:
    std::ostream& out_;
    std::string targetName_;
};

}  // namespace courier
