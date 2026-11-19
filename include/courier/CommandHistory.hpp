#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "courier/ShipmentCommand.hpp"

namespace courier {

// Command pattern invoker: runs commands and keeps them on an undo stack and a redo stack.
// Every function either succeeds completely or throws and leaves the stacks and the shipments
// exactly as they were.
class CommandHistory {
public:
    static constexpr std::size_t kDefaultLimit = 100;
    static constexpr std::size_t kMaxLimit = 10'000;

    // Keeps the last kDefaultLimit commands.
    CommandHistory();
    // Keeps the last `limit` commands, 1..kMaxLimit; the oldest is dropped when one more comes.
    explicit CommandHistory(std::size_t limit);

    // Runs the command and makes it the next one to undo. Clears the redo stack, because the
    // undone commands no longer fit the new state. A null command throws std::invalid_argument.
    void execute(std::unique_ptr<ShipmentCommand> command);
    // Both return false when there is nothing to undo or redo.
    bool undo();
    bool redo();

    [[nodiscard]] bool canUndo() const;
    [[nodiscard]] bool canRedo() const;
    // The command undo() or redo() would act on next, or nullptr if there is none.
    [[nodiscard]] const ShipmentCommand* nextUndo() const;
    [[nodiscard]] const ShipmentCommand* nextRedo() const;
    [[nodiscard]] std::size_t undoCount() const;
    [[nodiscard]] std::size_t redoCount() const;
    [[nodiscard]] std::size_t getLimit() const;

private:
    std::size_t limit_;
    // The back of each vector is the top of the stack. Both reserve room for `limit_` commands up
    // front, so moving a command between them never allocates and therefore cannot fail.
    std::vector<std::unique_ptr<ShipmentCommand>> undoStack_;
    std::vector<std::unique_ptr<ShipmentCommand>> redoStack_;
};

}  // namespace courier
