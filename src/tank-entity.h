#pragma once

#include <cstdint>

enum class Direction : uint8_t
{
    North, NorthEast, East, SouthEast, South, SouthWest, West, NorthWest
};

struct vec2
{
public:
    vec2(uint8_t x, uint8_t y)
    :x_(x), y_(y)
    {}

    uint8_t x_;
    uint8_t y_;
};

class Tank
{
public:

    Tank(uint8_t x_start, uint8_t y_start, Direction start_dir);

    inline vec2 get_pos() const;

    inline Direction get_dir() const;

    inline Direction get_barrel_dir() const;

private:
    vec2 current_position_;
    Direction current_direction_;
    Direction barrel_direction_;
};

inline vec2 Tank::get_pos() const
{
    return current_position_;
}

inline Direction Tank::get_dir() const
{
    return current_direction_;
}

inline Direction Tank::get_barrel_dir() const
{
    return barrel_direction_;
}
