#pragma once

namespace courier {

// Outer size of a parcel in centimetres. Every side is always greater than 0 and at most
// kMaxSideCm.
class Dimensions {
public:
    static constexpr double kMaxSideCm = 500.0;

    Dimensions(double length, double width, double height);
    explicit Dimensions(double side);

    [[nodiscard]] double getLength() const;
    [[nodiscard]] double getWidth() const;
    [[nodiscard]] double getHeight() const;

    void setLength(double length);
    void setWidth(double width);
    void setHeight(double height);

    // Volume in cubic centimetres.
    [[nodiscard]] double volume() const;

private:
    double length_;
    double width_;
    double height_;
};

}  // namespace courier
