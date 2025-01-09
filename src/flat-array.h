#pragma once

#include <cstdint>

// This class provides a more efficient way to dynamically allocate
// a variable size 2D array in a flattened 1D array format to avoid
// an array of pointers setup.
//
// We take as inputs width and length, and create a flat array.
template <typename element>
class FlatArray
{
public:

    FlatArray FlatArray(uint8_t input_width, uint8_t input_length);

    ~FlatArray();

    // You should not be copying or moving this data structure.
    FlatArray(FlatArray && other) = delete;

    // You should not be copying or moving this data structure.
    FlatArray(const FlatArray & other) = delete;


private:
    uint8_t width_ = 0;
    uint8_t length_ = 0;
    element* array_;
}
