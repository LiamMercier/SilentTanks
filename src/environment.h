#pragma once

#include "flat-array.h"
#include "grid-cell.h"

// This class represents an environment, specifically,
// the size, shape, and features of an environment.
class Environment
{
public:
    Environment() = delete;

    Environment(uint8_t input_width, uint8_t input_length);

    // For directly reading file to the new buffer
    Environment(uint8_t input_width, uint8_t input_length, uint16_t total_entries);

    ~Environment();

    Environment(Environment && other) = delete;

    Environment(const Environment & other) = delete;

    // Pass through the 2D -> 1D mapping for flat-array.
    inline size_t idx(size_t x, size_t y) const;

    // Pass through operator overloads for flat-array
    GridCell& operator[](size_t index);

    const GridCell& operator[](size_t index) const;

    inline uint8_t get_width() const;
    inline uint8_t get_length() const;
    inline GridCell* get_array();

private:
    FlatArray<GridCell> environment_layout_;
};

inline size_t Environment::idx(size_t x, size_t y) const
{
    return environment_layout_.idx(x,y);
}

inline uint8_t Environment::get_width() const
{
    return environment_layout_.get_width();
}

inline uint8_t Environment::get_length() const
{
    return environment_layout_.get_length();
}

inline GridCell* Environment::get_array()
{
    return environment_layout_.get_array();
}
