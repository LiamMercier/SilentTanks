#pragma once

#include <cstdint>
#include <cstddef>

// This class provides a more efficient way to dynamically allocate
// a variable size 2D array in a flattened 1D array format to avoid
// an array of pointers setup.
//
// We take as inputs width and length, and create a flat array.
template <typename element>
class FlatArray
{
public:

    FlatArray(uint8_t input_width, uint8_t input_length);

    ~FlatArray();

    // You should not be copying or moving this data structure.
    FlatArray(FlatArray && other) = delete;

    // You should not be copying or moving this data structure.
    FlatArray(const FlatArray & other) = delete;

    // 2D -> 1D mapping for array access.
    inline size_t idx(size_t x, size_t y) const;

    element& operator[](size_t index);

    const element& operator[](size_t index) const;

private:
    uint8_t width_;
    uint8_t length_;
    element* array_;
};

template <typename element>
inline size_t FlatArray<element>::idx(size_t x, size_t y) const
{
    return x + (width_ * y);
}
