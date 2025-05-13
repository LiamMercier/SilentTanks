#pragma once

#include <cstdint>
#include <cstddef>
#include <new>
#include <cstring>
#include <iostream>

// This class provides a more efficient way to dynamically allocate
// a variable size 2D array in a flattened 1D array format to avoid
// an array of pointers setup.
//
// We take as inputs width and height, and create a flat array.
template <typename element>
class FlatArray
{
public:

    FlatArray() noexcept;

    // Create the width by height array and zero it out
    FlatArray(uint8_t input_width, uint8_t input_height);

    // Create a flat 1D array WITHOUT zeroing.
    // Use FlatArray(width, height) unless you have a reason to do this!
    FlatArray(uint8_t input_width, uint8_t input_height, uint16_t total_entries);

    ~FlatArray();

    FlatArray(FlatArray && other) noexcept;

    FlatArray& operator=(FlatArray && other) noexcept;

    // copy/move ctor
    FlatArray<element>& operator=(const FlatArray& other);

    FlatArray(const FlatArray& other);

    // 2D -> 1D mapping for array access.
    inline size_t idx(size_t x, size_t y) const;

    element& operator[](size_t index);

    const element& operator[](size_t index) const;

    inline uint8_t get_width() const;

    inline uint8_t get_height() const;

    inline element* get_array();

    inline void set_width(uint8_t width);

    inline void set_height(uint8_t height);

private:
    uint8_t width_;
    uint8_t height_;
    element* array_;
};

template <typename element>
inline size_t FlatArray<element>::idx(size_t x, size_t y) const
{
    return x + (width_ * y);
}

template <typename element>
inline uint8_t FlatArray<element>::get_width() const
{
    return width_;
}

template <typename element>
inline uint8_t FlatArray<element>::get_height() const
{
    return height_;
}

template <typename element>
inline element* FlatArray<element>::get_array()
{
    return array_;
}

template <typename element>
inline void FlatArray<element>::set_width(uint8_t width)
{
    width_ = width;
}

template <typename element>
inline void FlatArray<element>::set_height(uint8_t height)
{
    height_ = height;
}
