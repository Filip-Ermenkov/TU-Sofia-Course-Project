#include "courier/CommandHistory.hpp"

#include <format>
#include <stdexcept>
#include <utility>

#include "courier/ValidationException.hpp"

namespace courier {

namespace {

std::size_t requireLimit(std::size_t limit) {
    if (limit == 0 || limit > CommandHistory::kMaxLimit) {
        throw ValidationException(std::format("Invalid undo limit: {}", limit));
    }
    return limit;
}

}  // namespace

CommandHistory::CommandHistory() : CommandHistory(kDefaultLimit) {}

CommandHistory::CommandHistory(std::size_t limit) : limit_(requireLimit(limit)) {
    undoStack_.reserve(limit_);
    redoStack_.reserve(limit_);
}

// The command runs first. If it throws, nothing has been touched yet; after it succeeds only
// steps that cannot fail are left.
void CommandHistory::execute(std::unique_ptr<ShipmentCommand> command) {
    if (!command) {
        throw std::invalid_argument("CommandHistory::execute needs a command");
    }
    command->execute();
    if (undoStack_.size() == limit_) {
        undoStack_.erase(undoStack_.begin());
    }
    undoStack_.push_back(std::move(command));
    redoStack_.clear();
}

bool CommandHistory::undo() {
    if (undoStack_.empty()) {
        return false;
    }
    undoStack_.back()->undo();
    redoStack_.push_back(std::move(undoStack_.back()));
    undoStack_.pop_back();
    return true;
}

bool CommandHistory::redo() {
    if (redoStack_.empty()) {
        return false;
    }
    redoStack_.back()->execute();
    undoStack_.push_back(std::move(redoStack_.back()));
    redoStack_.pop_back();
    return true;
}

bool CommandHistory::canUndo() const { return !undoStack_.empty(); }

bool CommandHistory::canRedo() const { return !redoStack_.empty(); }

const ShipmentCommand* CommandHistory::nextUndo() const {
    return undoStack_.empty() ? nullptr : undoStack_.back().get();
}

const ShipmentCommand* CommandHistory::nextRedo() const {
    return redoStack_.empty() ? nullptr : redoStack_.back().get();
}

std::size_t CommandHistory::undoCount() const { return undoStack_.size(); }

std::size_t CommandHistory::redoCount() const { return redoStack_.size(); }

std::size_t CommandHistory::getLimit() const { return limit_; }

}  // namespace courier
