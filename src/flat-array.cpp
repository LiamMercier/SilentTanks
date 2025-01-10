#include "flat-array.h"
#include "grid-cell.h"

template <typename element>
FlatArray<element>::FlatArray(uint8_t input_width, uint8_t input_length)
:width_(input_width), length_(input_length), array_(new (std::nothrow) element[input_width * input_length])
{
    // check that memory allocation succeeded
    if (array_ == nullptr)
    {
        std::cerr << "flat-array: Memory allocation failed\n";
        return;
    }

    // zero out the matrix
    std::memset(array_, 0, input_width * input_length * sizeof(element));

}

template <typename element>
FlatArray<element>::FlatArray(uint8_t input_width, uint8_t input_length, uint16_t total_entries)
:width_(input_width), length_(input_length), array_(new (std::nothrow) element[total_entries])
{

    // check that memory allocation succeeded
    if (array_ == nullptr)
    {
        std::cerr << "flat-array: Memory allocation failed\n";
        return;
    }
    // We now have a buffer of data (that hasn't been filled with zeros) which we
    // can fill directly (i.e using a file) instead of doing it twice.
}

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
