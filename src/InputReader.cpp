#include "courier/InputReader.hpp"

#include <format>
#include <iostream>

#include "courier/InputParsing.hpp"

namespace courier {

InputReader::InputReader() : InputReader(std::cin, std::cout) {}

InputReader::InputReader(std::istream& in, std::ostream& out) : in_(in), out_(out) {}

std::optional<std::string> InputReader::readLine(std::string_view prompt) {
    out_ << prompt << std::flush;
    std::string line;
    if (!std::getline(in_, line)) {
        out_ << '\n';  // the user's Enter is missing, so end the prompt line here
        return std::nullopt;
    }
    return line;
}

bool InputReader::atEnd() const { return in_.eof() || in_.fail(); }

template <typename T, typename Parser>
std::optional<T> InputReader::ask(std::string_view prompt, std::optional<T> fallback,
                                  std::string_view defaultText, Parser parse,
                                  std::string_view hint) {
    const std::string shown = defaultText.empty() ? std::format("{}: ", prompt)
                                                  : std::format("{} [{}]: ", prompt, defaultText);
    while (const auto line = readLine(shown)) {
        const std::string_view text = trim(*line);
        if (text.empty()) {
            return fallback;
        }
        if (auto value = parse(text)) {
            return value;
        }
        out_ << "  " << hint << '\n';
    }
    return std::nullopt;
}

namespace {

std::optional<std::string> anyText(std::string_view text) { return std::string(text); }

}  // namespace

std::optional<std::string> InputReader::askText(std::string_view prompt) {
    return ask<std::string>(prompt, std::nullopt, "", anyText, "");
}

std::optional<std::string> InputReader::askText(std::string_view prompt, std::string_view current) {
    return ask<std::string>(prompt, std::string(current), current.empty() ? "none" : current,
                            anyText, "");
}

std::optional<double> InputReader::askNumber(std::string_view prompt) {
    return ask<double>(prompt, std::nullopt, "", parseNumber, "Please enter a number, e.g. 2.5.");
}

std::optional<double> InputReader::askNumber(std::string_view prompt, double current) {
    return ask<double>(prompt, current, std::format("{}", current), parseNumber,
                       "Please enter a number, e.g. 2.5.");
}

std::optional<Money> InputReader::askMoney(std::string_view prompt) {
    return ask<Money>(prompt, std::nullopt, "", parseMoney,
                      "Please enter an amount with at most two decimals, e.g. 12.50.");
}

std::optional<Money> InputReader::askMoney(std::string_view prompt, Money current) {
    return ask<Money>(prompt, current, current.toString(), parseMoney,
                      "Please enter an amount with at most two decimals, e.g. 12.50.");
}

std::optional<TrackingId> InputReader::askTrackingId(std::string_view prompt) {
    return ask<TrackingId>(prompt, std::nullopt, "", parseTrackingId,
                           "Please enter a tracking ID such as CR00000001.");
}

std::optional<std::string> InputReader::askCode(std::string_view prompt) {
    const auto text = askText(prompt);
    if (!text) {
        return std::nullopt;
    }
    return normalizeCode(*text);
}

std::optional<std::size_t> InputReader::askChoice(std::string_view prompt,
                                                  const std::vector<std::string_view>& options) {
    for (std::size_t i = 0; i < options.size(); ++i) {
        out_ << std::format("  {}  {}\n", i + 1, options[i]);
    }
    const auto inRange = [&options](std::string_view text) -> std::optional<std::size_t> {
        const auto number = parseInteger(text);
        if (!number || *number < 1 || static_cast<std::size_t>(*number) > options.size()) {
            return std::nullopt;
        }
        return static_cast<std::size_t>(*number) - 1;
    };
    return ask<std::size_t>(prompt, std::nullopt, "", inRange,
                            std::format("Please enter a number from 1 to {}.", options.size()));
}

bool InputReader::askYesNo(std::string_view question) {
    const auto yesOrNo = [](std::string_view text) -> std::optional<bool> {
        const std::string answer = toUpper(text);
        if (answer == "Y" || answer == "YES") {
            return true;
        }
        if (answer == "N" || answer == "NO") {
            return false;
        }
        return std::nullopt;
    };
    const auto shown = std::format("{} (y/N)", question);
    return ask<bool>(shown, false, "", yesOrNo, "Please answer y or n.").value_or(false);
}

}  // namespace courier
