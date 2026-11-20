#pragma once

#include <cstddef>
#include <istream>
#include <optional>
#include <string>
#include <vector>

namespace courier {

// Reads records in CSV format (RFC 4180), as written by CsvWriter. Quoted fields may contain
// commas, doubled quotes and line breaks. Lines may end in "\n" or "\r\n". The stream is not
// owned and must outlive the reader.
class CsvReader {
public:
    // Error messages call the source "input".
    explicit CsvReader(std::istream& in);
    // sourceName appears in error messages, e.g. the name of the file being read.
    CsvReader(std::istream& in, std::string sourceName);

    // Bound to one stream for its whole life, so it cannot be copied or reassigned.
    CsvReader(const CsvReader&) = delete;
    CsvReader& operator=(const CsvReader&) = delete;
    CsvReader(CsvReader&&) = delete;
    CsvReader& operator=(CsvReader&&) = delete;
    ~CsvReader() = default;

    [[nodiscard]] const std::string& getSourceName() const;
    // Line on which the last record read began, counting from 1; 0 before the first record.
    [[nodiscard]] std::size_t getRowLine() const;

    // The next record, or nothing at the end of the input. Throws StorageException, naming the
    // source and line, for a quote inside an unquoted field, text after a closing quote, a quoted
    // field that is never closed, or a stream that fails.
    [[nodiscard]] std::optional<std::vector<std::string>> readRow();

private:
    std::istream& in_;
    std::string sourceName_;
    std::size_t line_ = 1;  // line the next character is on
    std::size_t rowLine_ = 0;
};

}  // namespace courier
