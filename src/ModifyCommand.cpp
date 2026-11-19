#include "courier/ModifyCommand.hpp"

#include <cstddef>
#include <format>
#include <vector>

namespace courier {

namespace {

bool sameSize(const Dimensions& a, const Dimensions& b) {
    return a.getLength() == b.getLength() && a.getWidth() == b.getWidth() &&
           a.getHeight() == b.getHeight();
}

std::string sizeText(const Dimensions& size) {
    return std::format("{:.1f} x {:.1f} x {:.1f}", size.getLength(), size.getWidth(),
                       size.getHeight());
}

// The details are valid, so none of the setters can reject them and the change is never half
// done. None of them allocates either, so nothing can fail at all.
void apply(Package& package, const ParcelDetails& details) {
    package.setWeightKg(details.getWeightKg());
    package.setDimensions(details.getDimensions());
    package.setDeclaredValue(details.getDeclaredValue());
}

}  // namespace

ModifyCommand::ModifyCommand(Package& package, ParcelDetails details)
    : ShipmentCommand(package), previous_(package), details_(details) {}

ModifyCommand::ModifyCommand(Package& package, double weightKg, Dimensions dimensions,
                             Money declaredValue)
    : ModifyCommand(package, ParcelDetails(weightKg, dimensions, declaredValue)) {}

std::string ModifyCommand::describe() const {
    std::vector<std::string> changes;
    if (details_.getWeightKg() != previous_.getWeightKg()) {
        changes.push_back(std::format("weight {:.2f} -> {:.2f} kg", previous_.getWeightKg(),
                                      details_.getWeightKg()));
    }
    if (!sameSize(details_.getDimensions(), previous_.getDimensions())) {
        changes.push_back(std::format("size {} -> {} cm", sizeText(previous_.getDimensions()),
                                      sizeText(details_.getDimensions())));
    }
    if (details_.getDeclaredValue() != previous_.getDeclaredValue()) {
        changes.push_back(std::format("declared value {} -> {}",
                                      previous_.getDeclaredValue().toString(),
                                      details_.getDeclaredValue().toString()));
    }

    std::string text = std::format("Modify {}: ", getId().getValue());
    if (changes.empty()) {
        return text + "no changes";
    }
    for (std::size_t i = 0; i < changes.size(); ++i) {
        text += (i == 0 ? "" : "; ") + changes[i];
    }
    return text;
}

void ModifyCommand::doExecute(Package& package) { apply(package, details_); }

void ModifyCommand::doUndo(Package& package) { apply(package, previous_); }

}  // namespace courier
