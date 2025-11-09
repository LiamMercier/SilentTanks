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

#include "flat-array.h"
#include "grid-cell.h"

template <typename element>
FlatArray<element>::FlatArray() noexcept
:width_(0),
height_(0),
array_()
{

}

// Allocate an empty array.
template <typename element>
FlatArray<element>::FlatArray(uint8_t input_width,
                              uint8_t input_height)
:width_(input_width),
height_(input_height),
array_(size_t(input_width * input_height))
{
    // Fill with empty elements.
    std::fill(array_.begin(), array_.end(), element{});
}

template <typename element>
element & FlatArray<element>::operator[](size_t index)
{
    return array_[index];
}

template <typename element>
const element & FlatArray<element>::operator[](size_t index) const
{
    return array_[index];
}

// Explicit template instantiation.
template class FlatArray<GridCell>;
