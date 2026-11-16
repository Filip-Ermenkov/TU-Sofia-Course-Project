#include "courier/Dimensions.hpp"

#include "Validation.hpp"

namespace courier {

Dimensions::Dimensions(double length, double width, double height)
    : length_(requirePositive("length", length)),
      width_(requirePositive("width", width)),
      height_(requirePositive("height", height)) {}

Dimensions::Dimensions(double side) : Dimensions(side, side, side) {}

double Dimensions::getLength() const { return length_; }

double Dimensions::getWidth() const { return width_; }

double Dimensions::getHeight() const { return height_; }

void Dimensions::setLength(double length) { length_ = requirePositive("length", length); }

void Dimensions::setWidth(double width) { width_ = requirePositive("width", width); }

void Dimensions::setHeight(double height) { height_ = requirePositive("height", height); }

double Dimensions::volume() const { return length_ * width_ * height_; }

}  // namespace courier
