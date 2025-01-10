#pragma once

#include <cstdint>

enum class Direction : uint8_t
{
    North, NorthEast, East, SouthEast, South, SouthWest, West, NorthWest
};

struct vec2
{
    uint8_t x;
    uint8_t y;
};

class Tank
{
public:
    Tank(uint8_t x_start, uint8_t y_start);
private:
    vec2 current_position;
    Direction current_direction;
    Direction barrel_direction;
};
