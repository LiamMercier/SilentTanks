#include "flat-array.h"
#include "grid-cell.h"

template <typename element>
FlatArray<element>::FlatArray(uint8_t input_width, uint8_t input_height)
:width_(input_width), height_(input_height), array_(new (std::nothrow) element[input_width * input_height])
{
    // check that memory allocation succeeded
    if (array_ == nullptr)
    {
        std::cerr << "flat-array: Memory allocation failed\n";
        return;
    }

    // zero out the matrix
    std::memset(array_, 0, input_width * input_height * sizeof(element));

}

template <typename element>
FlatArray<element>::FlatArray(uint8_t input_width, uint8_t input_height, uint16_t total_entries)
:width_(input_width), height_(input_height), array_(new (std::nothrow) element[total_entries])
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
FlatArray<element>::FlatArray(FlatArray && other) noexcept
:width_(other.width_), height_(other.height_), array_(other.array_)
{
    other.array_ = nullptr;
    other.width_ = 0;
    other.height_ = 0;
}

template <typename element>
FlatArray<element>& FlatArray<element>::operator=(FlatArray && other) noexcept
{
    // stop self assignment
    if (this != &other)
    {
        // delete old array
        delete[] array_;

        // swap
        width_ = other.width_;
        height_ = other.height_;
        array_ = other.array_;

        other.array_ = nullptr;
        other.width_ = 0;
        other.height_ = 0;

    }

    return *this;
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
