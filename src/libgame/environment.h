#pragma once

#include "flat-array.h"
#include "grid-cell.h"
#include "tank-entity.h"

// This class represents an environment, specifically,
// the size, shape, and features of an environment.
class Environment
{
public:
    Environment() = delete;

    Environment(uint8_t input_width, uint8_t input_height);

    // For directly reading file to the new buffer
    Environment(uint8_t input_width, uint8_t input_height, uint16_t total_entries);

    ~Environment() = default;

    Environment(Environment &&) noexcept = default;

    Environment& operator=(Environment &&) noexcept = default;

    // Pass through the 2D -> 1D mapping for flat-array.
    inline size_t idx(size_t x, size_t y) const;

    inline size_t idx(const vec2 & pos) const;

    // Pass through operator overloads for flat-array
    GridCell& operator[](size_t index);

    const GridCell& operator[](size_t index) const;

    void print_view(Tank* tanks_list) const;

    inline uint8_t get_width() const;

    inline uint8_t get_height() const;

    inline GridCell* get_array();

    inline void set_width(uint8_t width);

    inline void set_height(uint8_t height);

    // "Default" constructor call
    inline static Environment make_empty();
private:
    // Explicit default constructor
    struct ForceDefault{};

    explicit Environment(ForceDefault);

private:
    FlatArray<GridCell> environment_layout_;
};

inline size_t Environment::idx(size_t x, size_t y) const
{
    return environment_layout_.idx(x,y);
}

inline size_t Environment::idx(const vec2 &pos) const
{
    return environment_layout_.idx(pos.x_, pos.y_);
}

inline uint8_t Environment::get_width() const
{
    return environment_layout_.get_width();
}

inline uint8_t Environment::get_height() const
{
    return environment_layout_.get_height();
}

inline GridCell* Environment::get_array()
{
    return environment_layout_.get_array();
}

inline Environment Environment::make_empty()
{
    return Environment(ForceDefault{});
}

inline void Environment::set_width(uint8_t width)
{
    environment_layout_.set_width(width);
}

inline void Environment::set_height(uint8_t height)
{
    environment_layout_.set_height(height);
}
