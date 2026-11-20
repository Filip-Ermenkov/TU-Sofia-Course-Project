#pragma once

#include <cstddef>
#include <istream>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

#include "courier/Money.hpp"
#include "courier/TrackingId.hpp"

namespace courier {

// Asks the user for values, one line at a time. A prompt that shows a value in [brackets] takes
// that value when the user just presses Enter; a prompt without one treats an empty line as
// "cancel". Text in the wrong format is answered with a hint and asked again. Every ask...
// function returns nothing when the user cancelled or the input ended (atEnd() tells which).
class InputReader {
public:
    // Reads std::cin and writes to std::cout.
    InputReader();
    // Both streams must outlive the reader.
    InputReader(std::istream& in, std::ostream& out);

    // One reader per pair of streams: copies would share the same input.
    InputReader(const InputReader&) = delete;
    InputReader& operator=(const InputReader&) = delete;
    InputReader(InputReader&&) = delete;
    InputReader& operator=(InputReader&&) = delete;
    ~InputReader() = default;

    // Shows the prompt as it is and reads one line; nothing if the input has ended.
    std::optional<std::string> readLine(std::string_view prompt);
    // True once the input has ended (Ctrl+Z on Windows, Ctrl+D on Linux, or the end of a file).
    [[nodiscard]] bool atEnd() const;

    // Text that must be given; the result is trimmed.
    std::optional<std::string> askText(std::string_view prompt);
    // Text with a default; an empty `current` is shown as [none] and returned as "".
    std::optional<std::string> askText(std::string_view prompt, std::string_view current);

    std::optional<double> askNumber(std::string_view prompt);
    std::optional<double> askNumber(std::string_view prompt, double current);

    std::optional<Money> askMoney(std::string_view prompt);
    std::optional<Money> askMoney(std::string_view prompt, Money current);

    std::optional<TrackingId> askTrackingId(std::string_view prompt);

    // A courier or office code, upper-cased (see normalizeCode).
    std::optional<std::string> askCode(std::string_view prompt);

    // Lists the options numbered from 1 and returns the position (from 0) of the one chosen.
    std::optional<std::size_t> askChoice(std::string_view prompt,
                                         const std::vector<std::string_view>& options);

    // Enter or the end of the input mean no.
    bool askYesNo(std::string_view question);

private:
    // The common loop of the ask... functions. `parse` returns nothing for text in the wrong
    // format; `fallback` is returned for an empty line (nothing = cancel). `defaultText`, if not
    // empty, is shown in brackets.
    template <typename T, typename Parser>
    std::optional<T> ask(std::string_view prompt, std::optional<T> fallback,
                         std::string_view defaultText, Parser parse, std::string_view hint);

    std::istream& in_;
    std::ostream& out_;
};

}  // namespace courier
