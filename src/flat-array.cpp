#include "flat-array.h"
#include "grid-cell.h"

template <typename element>
FlatArray<element>::FlatArray(uint8_t input_width, uint8_t input_length)
:width_(input_width), length_(input_length), array_(new element[input_width * input_length]())
{}

template <typename element>
FlatArray<element>::~FlatArray()
{
    delete[] array_;
}

template <typename element>
element& FlatArray<element>::operator[](size_t index)
{
    return array_[index];
}

template <typename element>
const element& FlatArray<element>::operator[](size_t index) const
{
    return array_[index];
}

// Explicit template instantiation.
template class FlatArray<GridCell>;
