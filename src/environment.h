#pragma once

#include "flat-array.h"
#include "grid-cell.h"

// This class represents an environment, specifically,
// the size, shape, and features of an environment.
class Environment
{
public:
    Environment(uint8_t input_width, uint8_t input_length);

    ~Environment();

    Environment(Environment && other) = delete;

    Environment(const Environment & other) = delete;

    // Pass through the 2D -> 1D mapping for flat-array.
    inline size_t idx(size_t x, size_t y) const;

    // Pass through operator overloads for flat-array
    GridCell& operator[](size_t index);

    const GridCell& operator[](size_t index) const;

private:
    FlatArray<GridCell> environment_layout_;
};

inline size_t Environment::idx(size_t x, size_t y) const
{
    return environment_layout_.idx(x,y);
}
