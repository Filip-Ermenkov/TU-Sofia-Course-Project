#include "courier/Dimensions.hpp"

#include <cmath>
#include <string>

#include "courier/ValidationException.hpp"

namespace courier {

namespace {

// NaN fails every comparison, so "value <= 0" alone would let it through.
double requirePositive(const std::string& field, double value) {
    if (!std::isfinite(value) || value <= 0.0) {
        throw ValidationException(field, value);
    }
    return value;
}

}  // namespace

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
