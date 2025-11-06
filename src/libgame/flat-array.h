// Copyright (c) 2025 Liam Mercier
//
// This file is part of SilentTanks.
//
// SilentTanks is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License Version 3.0
// as published by the Free Software Foundation.
//
// SilentTanks is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License v3.0
// for more details.
//
// You should have received a copy of the GNU Affero General Public License v3.0
// along with SilentTanks. If not, see <https://www.gnu.org/licenses/agpl-3.0.txt>

#pragma once

#include <cstdint>
#include <cstddef>
#include <new>
#include <cstring>
#include <iostream>
#include <vector>

// This class provides a more efficient way to dynamically allocate
// a variable size 2D array in a flattened 1D array format to avoid
// an array of pointers setup.
//
// We take as inputs width and height, and create a flat array for
// our game, which will hold GridCell elements.
template <typename element>
class FlatArray
{
public:

    FlatArray() noexcept;

    // Create the width by height array and zero it out
    FlatArray(uint8_t input_width,
              uint8_t input_height);

    // 2D -> 1D mapping for array access.
    inline size_t idx(size_t x, size_t y) const;

    element& operator[](size_t index);

    const element& operator[](size_t index) const;

    inline uint8_t get_width() const;

    inline uint8_t get_height() const;

    inline void set_width(uint8_t width);

    inline void set_height(uint8_t height);

private:
    uint8_t width_;
    uint8_t height_;
    std::vector<element> array_;
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
inline void FlatArray<element>::set_width(uint8_t width)
{
    width_ = width;
}

template <typename element>
inline void FlatArray<element>::set_height(uint8_t height)
{
    height_ = height;
}
