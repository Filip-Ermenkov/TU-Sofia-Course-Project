#include "courier/StatusObserver.hpp"

namespace courier {

// Defined here, not in the header, so the class has a key function and its vtable is emitted once.
StatusObserver::~StatusObserver() = default;

}  // namespace courier
