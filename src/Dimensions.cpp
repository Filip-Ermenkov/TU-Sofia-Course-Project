#include "courier/Dimensions.hpp"

#include "Validation.hpp"

namespace courier {

namespace {

double requireSide(const std::string& field, double value) {
    return requireInRange(field, value, Dimensions::kMaxSideCm);
}

}  // namespace

Dimensions::Dimensions(double length, double width, double height)
    : length_(requireSide("length", length)),
      width_(requireSide("width", width)),
      height_(requireSide("height", height)) {}

Dimensions::Dimensions(double side) : Dimensions(side, side, side) {}

double Dimensions::getLength() const { return length_; }

double Dimensions::getWidth() const { return width_; }

double Dimensions::getHeight() const { return height_; }

void Dimensions::setLength(double length) { length_ = requireSide("length", length); }

void Dimensions::setWidth(double width) { width_ = requireSide("width", width); }

void Dimensions::setHeight(double height) { height_ = requireSide("height", height); }

double Dimensions::volume() const { return length_ * width_ * height_; }

}  // namespace courier
