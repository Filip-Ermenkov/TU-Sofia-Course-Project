#include "courier/CsvReader.hpp"

#include <format>
#include <utility>

#include "courier/StorageException.hpp"

namespace courier {

namespace {

constexpr auto kEnd = std::istream::traits_type::eof();

}  // namespace

CsvReader::CsvReader(std::istream& in) : CsvReader(in, "input") {}

CsvReader::CsvReader(std::istream& in, std::string sourceName)
    : in_(in), sourceName_(std::move(sourceName)) {}

const std::string& CsvReader::getSourceName() const { return sourceName_; }

std::size_t CsvReader::getRowLine() const { return rowLine_; }

std::optional<std::vector<std::string>> CsvReader::readRow() {
    const auto failIfBroken = [this] {
        if (in_.bad()) {
            throw StorageException(std::format("Cannot read from {}", sourceName_));
        }
    };
    if (in_.peek() == kEnd) {
        failIfBroken();
        return std::nullopt;
    }
    rowLine_ = line_;

    // Where we are inside the current field.
    enum class State { Start, Unquoted, Quoted, AfterQuote };
    State state = State::Start;
    std::vector<std::string> fields;
    std::string field;
    while (true) {
        const auto next = in_.get();
        if (next == kEnd) {
            failIfBroken();
            if (state == State::Quoted) {
                throw StorageException(sourceName_, rowLine_, "quoted field is not closed");
            }
            fields.push_back(std::move(field));  // last line without a line break
            return fields;
        }
        const char c = static_cast<char>(next);
        if (state == State::Quoted) {
            if (c != '"') {
                field += c;
                if (c == '\n') {
                    ++line_;
                }
            } else if (in_.peek() == '"') {
                in_.get();
                field += '"';  // a doubled quote stands for one quote
            } else {
                state = State::AfterQuote;
            }
        } else if (c == ',') {
            fields.push_back(std::move(field));
            field.clear();
            state = State::Start;
        } else if (c == '\n') {
            ++line_;
            fields.push_back(std::move(field));
            return fields;
        } else if (c == '\r' && in_.peek() == '\n') {
            // Part of a "\r\n" line ending; the '\n' ends the record on the next pass.
        } else if (state == State::AfterQuote) {
            throw StorageException(sourceName_, line_, "unexpected text after a closing quote");
        } else if (c == '"') {
            if (state != State::Start) {
                throw StorageException(sourceName_, line_, "quote inside an unquoted field");
            }
            state = State::Quoted;
        } else {
            field += c;
            state = State::Unquoted;
        }
    }
}

}  // namespace courier
