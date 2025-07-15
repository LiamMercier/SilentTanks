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
